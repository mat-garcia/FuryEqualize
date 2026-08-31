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

    float inputLevelDb() const { return inputLevelDb_.load(); }
    float gainReductionDb() const { return gainReductionDb_.load(); }

    int enumerateDevices(struct FuryDeviceInfo* out, int maxCount); // forward decl via audio_engine.h

private:
    void audioThreadProc(std::string deviceId);
    void processBuffer(float* data, UINT32 frames, int channels, double sampleRate);
    void updateEqCoeffs(double sampleRate);

    std::atomic<bool> running_{false};
    std::atomic<bool> stopRequested_{false};
    std::thread thread_;
    int bufferFrames_ = 256;

    // DSP: 3 bandas (low-shelf ~150Hz, peak 3.5kHz, high-shelf 10kHz)
    dsp::StereoBiquad lowShelf_, peak_, highShelf_;
    dsp::Compressor compL_, compR_; // usaremos um compressor estéreo (compartilhado)
    dsp::Compressor::Params compParams_{};
    float eqLow_=0, eqPeak_=0, eqHigh_=0;

    std::atomic<float> inputLevelDb_{-120.f};
    std::atomic<float> gainReductionDb_{0.f};

    // WASAPI handles (apenas na thread de áudio)
    IMMDeviceEnumerator* enumerator_ = nullptr;
};

WasapiEngine& globalEngine();

} // namespace fury::wasapi
