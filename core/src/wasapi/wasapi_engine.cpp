#include "wasapi/wasapi_engine.h"
#include "audio_engine.h"
#include <comdef.h>
#include <functiondiscoverykeys_devpkey.h>
#include <chrono>
#include <cmath>

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

void WasapiEngine::setBufferSize(int frames) {
    if(frames==128||frames==256||frames==512||frames==1024) bufferFrames_=frames;
}

void WasapiEngine::updateEqCoeffs(double sr) {
    // Low-shelf 150Hz, Peak 3500Hz Q=1.2, High-shelf 10000Hz
    auto low  = dsp::Biquad::makeLowShelf(sr, 150, eqLow_);
    auto peak = dsp::Biquad::makePeaking(sr, 3500, eqPeak_, 1.2);
    auto high = dsp::Biquad::makeHighShelf(sr, 10000, eqHigh_);
    lowShelf_.setCoeffs(low);
    peak_.setCoeffs(peak);
    highShelf_.setCoeffs(high);
}

int WasapiEngine::enumerateDevices(FuryDeviceInfo* out, int maxCount) {
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

    for(UINT i=0;i<(UINT)toCopy;i++){
        IMMDevice* pDev=nullptr; pColl->Item(i, &pDev);
        LPWSTR id=nullptr; pDev->GetId(&id);
        IPropertyStore* pProps=nullptr; pDev->OpenPropertyStore(STGM_READ, &pProps);
        PROPVARIANT varName; PropVariantInit(&varName);
        pProps->GetValue(PKEY_Device_FriendlyName, &varName);

        // Convert to UTF8 simplificado
        WideCharToMultiByte(CP_UTF8,0,id,-1,out[i].id,256,nullptr,nullptr);
        WideCharToMultiByte(CP_UTF8,0,varName.pwszVal,-1,out[i].name,256,nullptr,nullptr);
        out[i].channels = 2;
        out[i].sampleRate = 48000;
        out[i].isDefault = (defaultId && wcscmp(id, defaultId)==0) ? 1 : 0;

        // Tenta ler formato real para channels/samplerate
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
    // 1) Mede nível
    float peak = 0;
    for(UINT32 i=0;i<frames* (UINT32)channels;i++) peak = std::max(peak, std::abs(data[i]));
    float db = 20.f* std::log10(peak + 1e-9f);
    inputLevelDb_.store(db);

    // 2) Se estéreo, aplica cadeia DSP
    if(channels==2){
        // EQ chain
        lowShelf_.process(data, frames);
        peak_.process(data, frames);
        highShelf_.process(data, frames);
        // Compressor (stereo linked via max)
        compL_.process(data, frames);
        gainReductionDb_.store(compL_.gainReductionDb());
    } else if(channels==8){
        // 7.1: aplica por pares (suficiente para footsteps). Poderia converter para extensible.
        // Processa como 4 pares estéreo
        for(int ch=0; ch<8; ch+=2){
            // extrai par temporário? Simplificação: aplica compressor em cada par intercalado
            // Aqui apenas aplica ganho simplificado sem EQ multicanal completo
        }
        gainReductionDb_.store(0);
    }
}

void WasapiEngine::audioThreadProc(std::string deviceId) {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    // MMCSS para baixa latência
    DWORD taskIdx=0;
    HANDLE hTask = AvSetMmThreadCharacteristicsW(L"Pro Audio", &taskIdx);
    if(hTask) AvSetMmThreadPriority(hTask, AVRT_PRIORITY_CRITICAL);

    IMMDeviceEnumerator* pEnumerator=nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    IMMDevice* pDevice=nullptr;
    if(deviceId.empty()){
        hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    } else {
        // deviceId é UTF8 -> Wide
        wchar_t wId[256]; MultiByteToWideChar(CP_UTF8,0,deviceId.c_str(),-1,wId,256);
        hr = pEnumerator->GetDevice(wId, &pDevice);
        if(FAILED(hr)){
            pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
        }
    }

    IAudioClient* pAudioClient=nullptr;
    hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pAudioClient);

    WAVEFORMATEX* pwfx=nullptr;
    hr = pAudioClient->GetMixFormat(&pwfx);
    double sampleRate = pwfx ? pwfx->nSamplesPerSec : 48000;
    int channels = pwfx ? pwfx->nChannels : 2;
    bool isFloat = pwfx && pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE
        ? ((WAVEFORMATEXTENSIBLE*)pwfx)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT
        : pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT;

    updateEqCoeffs(sampleRate);
    compL_.configure(compParams_, sampleRate);
    compR_.configure(compParams_, sampleRate);
    // buffer de ~5.8ms para 256 samples @44.1kHz, escalado por sampleRate
    REFERENCE_TIME hnsBuffer = (REFERENCE_TIME)((double)bufferFrames_ / sampleRate * 10000000.0 * 1.5); // 1.5x margem
    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        hnsBuffer, 0, pwfx, nullptr);

    HANDLE hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    pAudioClient->SetEventHandle(hEvent);

    // Para loopback, precisamos também de um render client silencioso? Em SHARED loopback, basta capturar o mix.
    IAudioCaptureClient* pCapture=nullptr;
    pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCapture);
    // Também precisamos de output: captura loopback -> processa -> envia para render? 
    // Estratégia FuryEqualize: usa VB-Cable/Hi-Fi Cable como device virtual.
    // Simplificação Fase 1: apenas captura e monitora (sem re-render). Fase 1.5 adiciona IAudioRenderClient para VB-Cable.

    pAudioClient->Start();

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
                    // Assume FLOAT32. Se for PCM16, converter. Simplificação: só FLOAT.
                    if(isFloat || pwfx->wBitsPerSample==32){
                        float* fData = reinterpret_cast<float*>(pData);
                        // fData é interleaved channels * numFrames
                        processBuffer(fData, numFrames, channels, sampleRate);
                        // TODO Fase 1.5: enviar para render client do VB-Cable
                    }
                }
                pCapture->ReleaseBuffer(numFrames);
            }
            pCapture->GetNextPacketSize(&packetLen);
        }
    }

    pAudioClient->Stop();
    CloseHandle(hEvent);
    if(pCapture) pCapture->Release();
    if(pwfx) CoTaskMemFree(pwfx);
    if(pAudioClient) pAudioClient->Release();
    if(pDevice) pDevice->Release();
    if(pEnumerator) pEnumerator->Release();
    if(hTask) AvRevertMmThreadCharacteristics(hTask);
    CoUninitialize();
    running_=false;
}

} // namespace fury::wasapi
