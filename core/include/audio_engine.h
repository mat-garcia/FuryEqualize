#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
  #ifdef FURYCORE_EXPORTS
    #define FURY_API __declspec(dllexport)
  #else
    #define FURY_API __declspec(dllimport)
  #endif
#else
  #define FURY_API
#endif

#include <stdint.h>

// Códigos de retorno
#define FURY_OK 0
#define FURY_ERR_NOT_INITIALIZED -1
#define FURY_ERR_ALREADY_RUNNING -2
#define FURY_ERR_DEVICE_NOT_FOUND -3
#define FURY_ERR_WASAPI -10
#define FURY_ERR_INVALID_PARAM -20

// --- Ciclo de vida ---
FURY_API int32_t AudioEngine_Start(const char* deviceId); // nullptr = default loopback
FURY_API int32_t AudioEngine_Stop();
FURY_API int32_t AudioEngine_IsRunning(); // 0/1

// --- Configuração DSP ---
// threshold dB (ex: -30.0), ratio (ex: 4.0 = 4:1), attack/release em ms, makeup em dB
FURY_API int32_t AudioEngine_SetPreset(float thresholdDb, float ratio, float attackMs, float releaseMs, float makeupDb);
// EQ: ganhos em dB para low-shelf (~150Hz), peak (2-6kHz), high-shelf (~10kHz)
FURY_API int32_t AudioEngine_SetEq(float lowShelfGainDb, float peakGainDb, float highShelfGainDb);

// Buffer: 128, 256, 512, 1024
FURY_API int32_t AudioEngine_SetBuffer(int32_t bufferSize);
FURY_API int32_t AudioEngine_GetBuffer();

// --- Devices ---
typedef struct {
    char id[256];
    char name[256];
    int32_t channels; // 2 ou 8
    int32_t sampleRate;
    int32_t isDefault;
} FuryDeviceInfo;

// Retorna qtd de devices. Se outDevices != nullptr, preenche até maxCount.
FURY_API int32_t AudioEngine_GetDevices(FuryDeviceInfo* outDevices, int32_t maxCount);

// --- Telemetry ---
FURY_API float AudioEngine_GetInputLevelDb();  // pico atual
FURY_API float AudioEngine_GetGainReductionDb();

#ifdef __cplusplus
}
#endif
