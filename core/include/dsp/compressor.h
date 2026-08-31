#pragma once
#include <cmath>
#include <algorithm>

namespace fury::dsp {

// Compressor / Limiter para "Quiet Gunshots"
class Compressor {
public:
    struct Params {
        float thresholdDb = -24.0f; // -60..0
        float ratio = 4.0f;         // 1..20 (20 = limiter)
        float attackMs = 2.0f;      // 0.1..50
        float releaseMs = 80.0f;    // 10..500
        float makeupDb = 6.0f;      // 0..24
        float kneeDb = 6.0f;        // soft knee width
        double sampleRate = 48000.0;
    };

    void configure(const Params& p, double sampleRate) {
        params_ = p;
        params_.sampleRate = sampleRate;
        attackCoeff_  = std::exp(-1.0f / (float)(sampleRate * params_.attackMs / 1000.0));
        releaseCoeff_ = std::exp(-1.0f / (float)(sampleRate * params_.releaseMs / 1000.0));
        makeupLinear_ = std::pow(10.0f, params_.makeupDb / 20.0f);
    }

    void reset() { envelopeDb_ = -120.0f; gainReductionDb_ = 0.0f; }

    // Processa buffer interleaved estéreo in-place
    void process(float* interleaved, int frames) {
        for(int i=0;i<frames;i++){
            float l = interleaved[i*2];
            float r = interleaved[i*2+1];
            float inputAbs = std::max(std::abs(l), std::abs(r));
            float inputDb = 20.0f * std::log10(inputAbs + 1e-9f);

            // Envelope follower (peak + smoothing)
            float coeff = (inputDb > envelopeDb_) ? attackCoeff_ : releaseCoeff_;
            // smooth: env = coeff * env + (1-coeff)*input
            envelopeDb_ = coeff * envelopeDb_ + (1.0f - coeff) * inputDb;

            // Ganho por curva de compressão com soft knee
            float gr = computeGainReduction(envelopeDb_);
            gainReductionDb_ = gr; // para telemetry (último sample)
            float linearGain = std::pow(10.0f, gr / 20.0f) * makeupLinear_;

            interleaved[i*2]     = l * linearGain;
            interleaved[i*2 + 1] = r * linearGain;
        }
    }

    float gainReductionDb() const { return gainReductionDb_; }
    float envelopeDb() const { return envelopeDb_; }

private:
    float computeGainReduction(float inputDb) {
        float T = params_.thresholdDb;
        float R = params_.ratio;
        float W = params_.kneeDb;

        float overshoot = inputDb - T;
        float gr = 0.0f;
        if (overshoot <= -W/2) {
            gr = 0.0f;
        } else if (overshoot > -W/2 && overshoot < W/2) {
            // soft knee interpolation
            float x = overshoot + W/2;
            // compressão parcial dentro do knee
            gr = (1.0f / R - 1.0f) * (x * x) / (2.0f * W);
        } else {
            gr = overshoot * (1.0f / R - 1.0f);
        }
        return gr; // negativo
    }

    Params params_{};
    float attackCoeff_=0.9f, releaseCoeff_=0.99f;
    float makeupLinear_=1.0f;
    float envelopeDb_=-120.0f;
    float gainReductionDb_=0.0f;
};

} // namespace fury::dsp
