#define FURYCORE_EXPORTS
#include "audio_engine.h"
#include "wasapi/wasapi_engine.h"
#include <string>
#include <cstring>

int32_t AudioEngine_Start(const char* deviceId) {
    std::string id = deviceId ? deviceId : "";
    return fury::wasapi::globalEngine().start(id);
}
int32_t AudioEngine_Stop() { return fury::wasapi::globalEngine().stop(); }
int32_t AudioEngine_IsRunning() { return fury::wasapi::globalEngine().isRunning() ? 1 : 0; }

int32_t AudioEngine_SetPreset(float thr, float ratio, float attack, float release, float makeup) {
    if(ratio < 1 || ratio > 20) return FURY_ERR_INVALID_PARAM;
    fury::wasapi::globalEngine().setPreset(thr, ratio, attack, release, makeup);
    return FURY_OK;
}
int32_t AudioEngine_SetEq(float low, float peak, float high) {
    fury::wasapi::globalEngine().setEq(low, peak, high);
    return FURY_OK;
}
int32_t AudioEngine_SetBuffer(int32_t size) {
    if(size!=128 && size!=256 && size!=512 && size!=1024) return FURY_ERR_INVALID_PARAM;
    fury::wasapi::globalEngine().setBufferSize(size);
    return FURY_OK;
}
int32_t AudioEngine_GetBuffer() { return fury::wasapi::globalEngine().getBufferSize(); }

int32_t AudioEngine_SetRenderDevice(const char* renderDeviceId) {
    std::string id = renderDeviceId ? renderDeviceId : "";
    fury::wasapi::globalEngine().setRenderDevice(id);
    return FURY_OK;
}
int32_t AudioEngine_GetRenderDevice(char* outId, int32_t maxLen) {
    if(!outId || maxLen<=0) return FURY_ERR_INVALID_PARAM;
    auto s = fury::wasapi::globalEngine().getRenderDevice();
    strncpy_s(outId, maxLen, s.c_str(), _TRUNCATE);
    return (int32_t)s.size();
}

int32_t AudioEngine_GetDevices(FuryDeviceInfo* out, int32_t maxCount) {
    if(!out || maxCount<=0) return fury::wasapi::globalEngine().enumerateDevices(nullptr,0);
    return fury::wasapi::globalEngine().enumerateDevices(out, maxCount);
}
int32_t AudioEngine_SetMasterVolume(float db){ fury::wasapi::globalEngine().setMasterVolume(db); return FURY_OK; }
float AudioEngine_GetMasterVolume(){ return fury::wasapi::globalEngine().getMasterVolume(); }
int32_t AudioEngine_SetSniperPreset(const SniperPresetParams* p){ if(!p) return FURY_ERR_INVALID_PARAM; fury::wasapi::globalEngine().setSniperPreset(*p); return FURY_OK; }
int32_t AudioEngine_LoadSniperPresetJson(const char* path){ if(!path) return FURY_ERR_INVALID_PARAM; return fury::wasapi::globalEngine().loadSniperPresetJson(path); }
float AudioEngine_GetInputLevelDb() { return fury::wasapi::globalEngine().inputLevelDb(); }
float AudioEngine_GetGainReductionDb() { return fury::wasapi::globalEngine().gainReductionDb(); }
