#include "wasapi/wasapi_engine.h"
#include "audio_engine.h"
#include <comdef.h>
#include <functiondiscoverykeys_devpkey.h>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <vector>

namespace fury::wasapi {

WasapiEngine& globalEngine() {
    static WasapiEngine inst;
    return inst;
}

WasapiEngine::WasapiEngine() {
    compParams_.thresholdDb = -24;
    compParams_.ratio = 4;
    compParams_.attackMs = 2;
    compParams_.releaseMs = 80;
    compParams_.makeupDb = 6;
    compL_.configure(compParams_, 48000);
    compR_.configure(compParams_, 48000);
    updateEqCoeffs(48000);
    masterVolumeDb_=0; masterLinear_=1.0f;
    sniperPreset_=g_activeSniperPreset;
}

WasapiEngine::~WasapiEngine() { stop(); }

void WasapiEngine::setPreset(float thr, float ratio, float atk, float rel, float makeup) {
    compParams_.thresholdDb = thr;
    compParams_.ratio = std::clamp(ratio, 1.f, 20.f);
    compParams_.attackMs = std::clamp(atk, 0.1f, 100.f);
    compParams_.releaseMs = std::clamp(rel, 10.f, 500.f);
    compParams_.makeupDb = makeup;
    compL_.configure(compParams_, 48000);
    compR_.configure(compParams_, 48000);
}

void WasapiEngine::setEq(float low, float peak, float high) {
    eqLow_ = low; eqPeak_ = peak; eqHigh_ = high;
    updateEqCoeffs(48000);
}
void WasapiEngine::setMasterVolume(float db){ masterVolumeDb_=db; masterLinear_=std::pow(10.0f, db/20.0f); }
void WasapiEngine::setSniperPreset(const SniperPresetParams& p){
    sniperPreset_=p;
    g_activeSniperPreset=p;
    // Mapeia Level Tracking para compressor: propThresh/propRatio/attack/release
    setPreset(p.propThresh, p.propRatio, p.attackMs, p.releaseMs, 0);
    // Footstep boost -> EQ: low = subWeight, peak = stepLift
    setEq(p.subWeight*10, p.stepLift, p.coherenceMax*10);
    setMasterVolume(p.masterVolumeDb);
}
int WasapiEngine::loadSniperPresetJson(const std::string& path){
    // JSON loader simples — para MVP, apenas seta preset padrão se arquivo existir
    // Leitura real via nlohmann/json seria ideal; por enquanto, se arquivo existe, aplica preset ativo
    FILE* f=nullptr;
    fopen_s(&f, path.c_str(), "rb");
    if(!f) return FURY_ERR_DEVICE_NOT_FOUND;
    fclose(f);
    // Aqui deserializaria JSON para SniperPresetParams; simplificado: usa preset já carregado
    setSniperPreset(sniperPreset_);
    return FURY_OK;
}

void WasapiEngine::setBufferSize(int frames) {
    if(frames==128||frames==256||frames==512||frames==1024) bufferFrames_=frames;
}

void WasapiEngine::updateEqCoeffs(double sr) {
    auto low  = dsp::Biquad::makeLowShelf(sr, 150, eqLow_);
    auto peak = dsp::Biquad::makePeaking(sr, 3500, eqPeak_, 1.2);
    auto high = dsp::Biquad::makeHighShelf(sr, 10000, eqHigh_);
    lowShelf_.setCoeffs(low);
    peak_.setCoeffs(peak);
    highShelf_.setCoeffs(high);
    for(auto &b : mcLow_) b.setCoeffs(low);
    for(auto &b : mcPeak_) b.setCoeffs(peak);
    for(auto &b : mcHigh_) b.setCoeffs(high);
}

void WasapiEngine::pcm16ToFloat(const int16_t* src, float* dst, size_t samples) {
    for(size_t i=0;i<samples;i++) dst[i] = src[i] / 32768.0f;
}
void WasapiEngine::floatToPcm16(const float* src, int16_t* dst, size_t samples) {
    for(size_t i=0;i<samples;i++) {
        float v = std::clamp(src[i], -1.0f, 1.0f);
        dst[i] = (int16_t)std::lround(v * 32767.0f);
    }
}

std::string WasapiEngine::findVirtualCableDevice() {
    if(!renderDeviceId_.empty() && renderDeviceId_ != "auto") return renderDeviceId_;
    // Busca heurística por VB-Cable / Hi-Fi Cable
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    IMMDeviceEnumerator* pEnum=nullptr;
    if(FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum))) return "";
    IMMDeviceCollection* pColl=nullptr;
    if(FAILED(pEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pColl))) { pEnum->Release(); return ""; }
    UINT count=0; pColl->GetCount(&count);
    std::string found;
    for(UINT i=0;i<count;i++){
        IMMDevice* pDev=nullptr; pColl->Item(i, &pDev);
        IPropertyStore* pProps=nullptr; pDev->OpenPropertyStore(STGM_READ, &pProps);
        PROPVARIANT varName; PropVariantInit(&varName);
        pProps->GetValue(PKEY_Device_FriendlyName, &varName);
        char name[256]={0};
        if(varName.pwszVal) WideCharToMultiByte(CP_ACP,0,varName.pwszVal,-1,name,256,nullptr,nullptr);
        std::string n = name;
        std::string lower=n; std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if(lower.find("cable")!=std::string::npos || lower.find("vb-audio")!=std::string::npos || lower.find("hi-fi")!=std::string::npos || lower.find("hifi")!=std::string::npos){
            LPWSTR id=nullptr; pDev->GetId(&id);
            char id8[256]={0}; WideCharToMultiByte(CP_ACP,0,id,-1,id8,256,nullptr,nullptr);
            found = id8;
            CoTaskMemFree(id);
            PropVariantClear(&varName); pProps->Release(); pDev->Release();
            break;
        }
        PropVariantClear(&varName); pProps->Release(); pDev->Release();
    }
    pColl->Release(); pEnum->Release();
    return found;
}

int WasapiEngine::enumerateDevices(::FuryDeviceInfo* out, int maxCount) {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    IMMDeviceEnumerator* pEnum=nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum);
    if(FAILED(hr)) return 0;
    IMMDeviceCollection* pColl=nullptr;
    hr = pEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pColl);
    if(FAILED(hr)){ pEnum->Release(); return 0;}
    UINT count=0; pColl->GetCount(&count);
    int toCopy = std::min<int>(count, maxCount);
    IMMDevice* pDefault=nullptr; LPWSTR defaultId=nullptr;
    pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &pDefault);
    if(pDefault) pDefault->GetId(&defaultId);

    for(int i=0;i<toCopy;i++){
        IMMDevice* pDev=nullptr; pColl->Item((UINT)i, &pDev);
        LPWSTR id=nullptr; pDev->GetId(&id);
        IPropertyStore* pProps=nullptr; pDev->OpenPropertyStore(STGM_READ, &pProps);
        PROPVARIANT varName; PropVariantInit(&varName);
        pProps->GetValue(PKEY_Device_FriendlyName, &varName);
        WideCharToMultiByte(CP_ACP,0,id,-1,out[i].id,256,nullptr,nullptr);
        WideCharToMultiByte(CP_ACP,0,varName.pwszVal,-1,out[i].name,256,nullptr,nullptr);
        out[i].channels = 2;
        out[i].sampleRate = 48000;
        out[i].isDefault = (defaultId && wcscmp(id, defaultId)==0) ? 1 : 0;
        IAudioClient* pClient=nullptr;
        if(SUCCEEDED(pDev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pClient))){
            WAVEFORMATEX* pwfx=nullptr;
            if(SUCCEEDED(pClient->GetMixFormat(&pwfx))){
                out[i].channels = pwfx->nChannels;
                out[i].sampleRate = pwfx->nSamplesPerSec;
                CoTaskMemFree(pwfx);
            }
            pClient->Release();
        }
        CoTaskMemFree(id);
        PropVariantClear(&varName);
        pProps->Release(); pDev->Release();
    }
    if(defaultId) CoTaskMemFree(defaultId);
    if(pDefault) pDefault->Release();
    pColl->Release(); pEnum->Release();
    return (int)count;
}

int WasapiEngine::start(const std::string& deviceId) {
    if(running_.load()) return FURY_ERR_ALREADY_RUNNING;
    stopRequested_=false;
    running_=true;
    thread_ = std::thread(&WasapiEngine::audioThreadProc, this, deviceId);
    return FURY_OK;
}

int WasapiEngine::stop() {
    if(!running_.load() && !thread_.joinable()) return FURY_OK;
    stopRequested_=true;
    if(thread_.joinable()) thread_.join();
    running_=false;
    return FURY_OK;
}

void WasapiEngine::processBuffer(float* data, UINT32 frames, int channels, double sampleRate) {
    (void)sampleRate;
    float peak = 0;
    for(UINT32 i=0;i<frames * (UINT32)channels;i++) peak = std::max(peak, std::abs(data[i]));
    float db = 20.f* std::log10(peak + 1e-9f);
    inputLevelDb_.store(db);

    if(channels==2){
        lowShelf_.process(data, frames);
        peak_.process(data, frames);
        highShelf_.process(data, frames);
        compL_.process(data, frames);
        gainReductionDb_.store(compL_.gainReductionDb());
    } else if(channels==1){
        // mono -> aplica por sample
        for(UINT32 f=0; f<frames; f++){
            float s = data[f];
            s = mcLow_[0].process(s);
            s = mcPeak_[0].process(s);
            s = mcHigh_[0].process(s);
            data[f]=s;
        }
        // compressor mono (reusa compL com buffer mono)
        // para mono, cria temp intercalado mono->estéreo fake? Simplifica: aplica ganho do compressor manual
        // Usa compL_ em modo mono: precisamos processar como se fosse 1ch
        // Workaround: duplica para estéreo temp
        // Para manter simples, só aplica makeup sem compressão dinâmica em mono
        gainReductionDb_.store(0);
    } else {
        // Sauda 7.1 (8ch) e 5.1: downmix para estéreo com ganhos Sauda + processa estéreo
        // Se for 7.1 (8ch) vindo do VB-Cable Sauda, faz fold correto para fone estéreo
        if(channels==8){
            float stereo[2048];
            if(frames>1024) frames=1024;
            for(UINT32 i=0;i<frames;i++){
                float fl = data[i*8+0];
                float fr = data[i*8+1];
                float fc = data[i*8+2];
                float lfe= data[i*8+3]*0.5f;
                float bl = data[i*8+4];
                float br = data[i*8+5];
                float sl = data[i*8+6];
                float sr = data[i*8+7];
                float l = fl + fc*0.707f + bl*0.707f + sl*0.707f + lfe;
                float r = fr + fc*0.707f + br*0.707f + sr*0.707f + lfe;
                stereo[i*2]=l; stereo[i*2+1]=r;
            }
            // Processa estéreo downmixado com chain principal
            lowShelf_.process(stereo, frames);
            peak_.process(stereo, frames);
            highShelf_.process(stereo, frames);
            compL_.process(stereo, frames);
            gainReductionDb_.store(compL_.gainReductionDb());
            // Copia de volta como estéreo compactado (2*frames) no início do buffer
            for(UINT32 i=0;i<frames*2;i++) data[i]=stereo[i];
            // Marca que agora é estéreo para master (caller deve tratar como 2ch)
            // Para manter compatível, não altera channels param, mas master abaixo trata 2*frames
        } else {
            int ch = std::min<int>(channels, 8);
            for(UINT32 f=0; f<frames; f++){
                for(int c=0;c<ch;c++){
                    float s = data[f*channels + c];
                    s = mcLow_[c].process(s);
                    s = mcPeak_[c].process(s);
                    s = mcHigh_[c].process(s);
                    data[f*channels + c] = s;
                }
            }
            float maxAbs = 0;
            for(UINT32 i=0;i<frames* (UINT32)channels;i++) maxAbs = std::max(maxAbs, std::abs(data[i]));
            float dummy[2] = {maxAbs, maxAbs};
            float dummyBuf[2] = {dummy[0], dummy[1]};
            compL_.process(dummyBuf, 1);
            float gr = compL_.gainReductionDb();
            gainReductionDb_.store(gr);
            float lin = std::pow(10.0f, gr/20.0f) * std::pow(10.0f, compParams_.makeupDb/20.0f);
            if(ch != 2){
                for(UINT32 i=0;i<frames* (UINT32)channels;i++) data[i] *= lin;
            }
        }
    }
    // Master volume (Sniper preset) — para Sauda 7.1 downmix, aplica só nos 2*frames válidos
    int effectiveCh = (channels==8) ? 2 : (int)channels;
    if(masterLinear_ != 1.0f){
        for(UINT32 i=0;i<frames*(UINT32)effectiveCh;i++) data[i] *= masterLinear_;
        for(UINT32 i=0;i<frames*(UINT32)effectiveCh;i++) data[i] = std::clamp(data[i], -1.0f, 1.0f);
    }
}

void WasapiEngine::audioThreadProc(std::string captureDeviceId) {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    DWORD taskIdx=0;
    HANDLE hTask = AvSetMmThreadCharacteristicsW(L"Pro Audio", &taskIdx);
    if(hTask) AvSetMmThreadPriority(hTask, AVRT_PRIORITY_CRITICAL);

    IMMDeviceEnumerator* pEnumerator=nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    IMMDevice* pCaptureDevice=nullptr;
    if(captureDeviceId.empty()){
        hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pCaptureDevice);
    } else {
        wchar_t wId[256]; MultiByteToWideChar(CP_UTF8,0,captureDeviceId.c_str(),-1,wId,256);
        hr = pEnumerator->GetDevice(wId, &pCaptureDevice);
        if(FAILED(hr)) hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pCaptureDevice);
    }

    // --- Capture client (loopback) ---
    IAudioClient* pCaptureClient=nullptr;
    hr = pCaptureDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pCaptureClient);
    WAVEFORMATEX* pwfxCapture=nullptr;
    hr = pCaptureClient->GetMixFormat(&pwfxCapture);
    double sampleRateCap = pwfxCapture ? pwfxCapture->nSamplesPerSec : 48000;
    int channelsCap = pwfxCapture ? pwfxCapture->nChannels : 2;
    bool isFloatCap = pwfxCapture && pwfxCapture->wFormatTag == WAVE_FORMAT_EXTENSIBLE
        ? ((WAVEFORMATEXTENSIBLE*)pwfxCapture)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT
        : pwfxCapture && pwfxCapture->wFormatTag == WAVE_FORMAT_IEEE_FLOAT;
    bool isPcm16Cap = pwfxCapture && pwfxCapture->wBitsPerSample==16;

    updateEqCoeffs(sampleRateCap);
    compL_.configure(compParams_, sampleRateCap);
    compR_.configure(compParams_, sampleRateCap);

    REFERENCE_TIME hnsBuffer = (REFERENCE_TIME)((double)bufferFrames_ / sampleRateCap * 10000000.0 * 1.5);
    hr = pCaptureClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        hnsBuffer, 0, pwfxCapture, nullptr);
    HANDLE hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    pCaptureClient->SetEventHandle(hEvent);
    IAudioCaptureClient* pCapture=nullptr;
    pCaptureClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCapture);

    // --- Render client (VB-Cable) ---
    IAudioClient* pRenderClient=nullptr;
    IAudioRenderClient* pRender=nullptr;
    WAVEFORMATEX* pwfxRender=nullptr;
    HANDLE hRenderEvent=nullptr;
    bool useRender = false;
    std::string renderId = findVirtualCableDevice();
    IMMDevice* pRenderDevice=nullptr;
    uint32_t renderBufferFrames=0;

    if(renderDeviceId_ != "none" && !renderId.empty()){
        wchar_t wRenderId[256]; MultiByteToWideChar(CP_UTF8,0,renderId.c_str(),-1,wRenderId,256);
        if(SUCCEEDED(pEnumerator->GetDevice(wRenderId, &pRenderDevice))){
            if(SUCCEEDED(pRenderDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pRenderClient))){
                hr = pRenderClient->GetMixFormat(&pwfxRender);
                // Tenta inicializar com formato do render; se sampleRate diferente, precisará resample (placeholder)
                if(SUCCEEDED(hr)){
                    double srRender = pwfxRender->nSamplesPerSec;
                    // Se sample rates diferem, loga mas continua (resample placeholder: apenas ajusta coeff)
                    if(std::abs(srRender - sampleRateCap) > 1.0){
                        // TODO: resampler real (Speex/SRC). Por enquanto, mantém sem correção — pode haver drift
                    }
                    hr = pRenderClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, hnsBuffer, 0, pwfxRender, nullptr);
                    if(SUCCEEDED(hr)){
                        pRenderClient->GetService(__uuidof(IAudioRenderClient), (void**)&pRender);
                        pRenderClient->GetBufferSize(&renderBufferFrames);
                        hRenderEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
                        pRenderClient->SetEventHandle(hRenderEvent);
                        useRender = true;
                        // Preenche buffer inicial com silêncio
                        BYTE* pData=nullptr;
                        if(SUCCEEDED(pRender->GetBuffer(renderBufferFrames, &pData))){
                            memset(pData, 0, renderBufferFrames * pwfxRender->nBlockAlign);
                            pRender->ReleaseBuffer(renderBufferFrames, 0);
                        }
                    }
                }
            }
        }
    }

    pCaptureClient->Start();
    if(useRender) pRenderClient->Start();

    std::vector<float> floatBuf;
    std::vector<int16_t> pcm16Buf;

    while(!stopRequested_.load()){
        DWORD wait = WaitForSingleObject(hEvent, 200);
        if(wait != WAIT_OBJECT_0) continue;

        UINT32 packetLen=0;
        pCapture->GetNextPacketSize(&packetLen);
        while(packetLen > 0){
            BYTE* pData=nullptr;
            UINT32 numFrames=0;
            DWORD flags=0;
            hr = pCapture->GetBuffer(&pData, &numFrames, &flags, nullptr, nullptr);
            if(SUCCEEDED(hr) && numFrames>0){
                if(!(flags & AUDCLNT_BUFFERFLAGS_SILENT) && pData){
                    float* fData=nullptr;
                    bool needsFree=false;
                    size_t totalSamples = (size_t)numFrames * channelsCap;

                    if(isFloatCap || (!isPcm16Cap && pwfxCapture->wBitsPerSample==32)){
                        fData = reinterpret_cast<float*>(pData);
                        // processa in-place (para Sauda 7.1 downmix para estéreo dentro)
                        processBuffer(fData, numFrames, channelsCap, sampleRateCap);
                        int outCh = (channelsCap==8) ? 2 : (int)channelsCap;
                        size_t outSamples = (size_t)numFrames * outCh;

                        // Envia para render se ativo
                        if(useRender && pRender){
                            UINT32 pad=0; pRenderClient->GetCurrentPadding(&pad);
                            UINT32 avail = renderBufferFrames > pad ? renderBufferFrames - pad : 0;
                            if(avail >= numFrames){
                                BYTE* pOut=nullptr;
                                if(SUCCEEDED(pRender->GetBuffer(numFrames, &pOut))){
                                    bool rIsFloat = pwfxRender->wFormatTag==WAVE_FORMAT_EXTENSIBLE
                                        ? ((WAVEFORMATEXTENSIBLE*)pwfxRender)->SubFormat==KSDATAFORMAT_SUBTYPE_IEEE_FLOAT
                                        : pwfxRender->wFormatTag==WAVE_FORMAT_IEEE_FLOAT;
                                    if(rIsFloat || pwfxRender->wBitsPerSample==32){
                                        memcpy(pOut, fData, numFrames * pwfxRender->nBlockAlign);
                                    } else if(pwfxRender->wBitsPerSample==16){
                                        floatToPcm16(fData, reinterpret_cast<int16_t*>(pOut), outSamples);
                                    } else {
                                        memcpy(pOut, fData, std::min<size_t>(numFrames * pwfxRender->nBlockAlign, outSamples * sizeof(float)));
                                    }
                                    pRender->ReleaseBuffer(numFrames, 0);
                                }
                            } else {
                                // buffer cheio — drop frame para manter latência baixa
                            }
                        }

                    } else if(isPcm16Cap){
                        if(floatBuf.size() < totalSamples) floatBuf.resize(totalSamples);
                        pcm16ToFloat(reinterpret_cast<int16_t*>(pData), floatBuf.data(), totalSamples);
                        processBuffer(floatBuf.data(), numFrames, channelsCap, sampleRateCap);
                        int outCh = (channelsCap==8) ? 2 : (int)channelsCap;
                        size_t outSamples = (size_t)numFrames * outCh;
                        if(useRender && pRender){
                            UINT32 pad=0; pRenderClient->GetCurrentPadding(&pad);
                            UINT32 avail = renderBufferFrames > pad ? renderBufferFrames - pad : 0;
                            if(avail >= numFrames){
                                BYTE* pOut=nullptr;
                                if(SUCCEEDED(pRender->GetBuffer(numFrames, &pOut))){
                                    bool rIsFloat = pwfxRender->wFormatTag==WAVE_FORMAT_EXTENSIBLE
                                        ? ((WAVEFORMATEXTENSIBLE*)pwfxRender)->SubFormat==KSDATAFORMAT_SUBTYPE_IEEE_FLOAT
                                        : pwfxRender->wFormatTag==WAVE_FORMAT_IEEE_FLOAT;
                                    if(rIsFloat){
                                        memcpy(pOut, floatBuf.data(), outSamples*sizeof(float));
                                    } else {
                                        floatToPcm16(floatBuf.data(), reinterpret_cast<int16_t*>(pOut), outSamples);
                                    }
                                    pRender->ReleaseBuffer(numFrames, 0);
                                }
                            }
                        }
                        fData = nullptr; // não usado
                    }
                    (void)needsFree;
                    (void)fData;
                }
                pCapture->ReleaseBuffer(numFrames);
            } else if(SUCCEEDED(hr)){
                pCapture->ReleaseBuffer(numFrames);
            }
            pCapture->GetNextPacketSize(&packetLen);
        }
    }

    pCaptureClient->Stop();
    if(useRender) pRenderClient->Stop();
    CloseHandle(hEvent);
    if(hRenderEvent) CloseHandle(hRenderEvent);
    if(pCapture) pCapture->Release();
    if(pRender) pRender->Release();
    if(pwfxCapture) CoTaskMemFree(pwfxCapture);
    if(pwfxRender) CoTaskMemFree(pwfxRender);
    if(pCaptureClient) pCaptureClient->Release();
    if(pRenderClient) pRenderClient->Release();
    if(pCaptureDevice) pCaptureDevice->Release();
    if(pRenderDevice) pRenderDevice->Release();
    if(pEnumerator) pEnumerator->Release();
    if(hTask) AvRevertMmThreadCharacteristics(hTask);
    CoUninitialize();
    running_=false;
}

} // namespace fury::wasapi
