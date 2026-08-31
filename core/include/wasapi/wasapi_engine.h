#pragma once
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <avrt.h>
#include <atomic>
#include <thread>
#include <string>
#include "../dsp/biquad.h"
#include "../dsp/compressor.h"
#include "../audio_engine.h"

namespace fury::wasapi {

class WasapiEngine {
public:
    WasapiEngine();
    ~WasapiEngine();

    int start(const std::string& deviceId);
    int stop();
    bool isRunning() const { return running_.load(); }

    void setPreset(float thresholdDb, float ratio, float attackMs, float releaseMs, float makeupDb);
    void setEq(float lowGain, float peakGain, float highGain);
    void setBufferSize(int frames); // 128/256/512/1024
    int  getBufferSize() const { return bufferFrames_; }
    void setRenderDevice(const std::string& renderId) { renderDeviceId_ = renderId; }
    std::string getRenderDevice() const { return renderDeviceId_; }

    float inputLevelDb() const { return inputLevelDb_.load(); }
    float gainReductionDb() const { return gainReductionDb_.load(); }

    int enumerateDevices(::FuryDeviceInfo* out, int maxCount);
    // Helper para auto-detectar VB-Cable / Hi-Fi Cable
    std::string findVirtualCableDevice();

private:
    void audioThreadProc(std::string captureDeviceId);
    void processBuffer(float* data, UINT32 frames, int channels, double sampleRate);
    void updateEqCoeffs(double sampleRate);
    // Conversão PCM16 <-> float (para devices não-float)
    static void pcm16ToFloat(const int16_t* src, float* dst, size_t samples);
    static void floatToPcm16(const float* src, int16_t* dst, size_t samples);

    std::atomic<bool> running_{false};
    std::atomic<bool> stopRequested_{false};
    std::thread thread_;
    int bufferFrames_ = 256;

    // DSP: 3 bandas (low-shelf ~150Hz, peak 3.5kHz, high-shelf 10kHz)
    dsp::StereoBiquad lowShelf_, peak_, highShelf_;
    // Para 7.1 (8ch) — 8 instâncias por banda
    std::array<dsp::Biquad, 8> mcLow_{}, mcPeak_{}, mcHigh_{};
    dsp::Compressor compL_, compR_; // estéreo; para 7.1 reusamos compL_ linked
    dsp::Compressor::Params compParams_{};
    float eqLow_=0, eqPeak_=0, eqHigh_=0;

    std::atomic<float> inputLevelDb_{-120.f};
    std::atomic<float> gainReductionDb_{0.f};
    std::string renderDeviceId_; // vazio = auto-detect VB-Cable, "none" = sem render (monitor only)

    // WASAPI handles (apenas na thread de áudio)
    IMMDeviceEnumerator* enumerator_ = nullptr;
};

WasapiEngine& globalEngine();

} // namespace fury::wasapi
