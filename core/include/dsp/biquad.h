#pragma once
#include <cmath>
#include <array>

namespace fury::dsp {

// Cookbook RBJ Audio EQ
struct BiquadCoeffs {
    float b0=1, b1=0, b2=0, a1=0, a2=0;
};

class Biquad {
public:
    Biquad() { reset(); }

    void setCoeffs(const BiquadCoeffs& c) { coeffs_ = c; }
    void reset() { z1_=0; z2_=0; }

    // Process single sample (Direct Form II transposed)
    inline float process(float x) {
        float y = coeffs_.b0 * x + z1_;
        z1_ = coeffs_.b1 * x - coeffs_.a1 * y + z2_;
        z2_ = coeffs_.b2 * x - coeffs_.a2 * y;
        return y;
    }

    // Geradores estáticos
    static BiquadCoeffs makeLowShelf(double Fs, double Fc, double gainDb, double S = 1.0);
    static BiquadCoeffs makeHighShelf(double Fs, double Fc, double gainDb, double S = 1.0);
    static BiquadCoeffs makePeaking(double Fs, double Fc, double gainDb, double Q);

private:
    BiquadCoeffs coeffs_{};
    float z1_=0, z2_=0;
};

// Cadeia estéreo (um biquad por canal)
class StereoBiquad {
public:
    void setCoeffs(const BiquadCoeffs& c) { l_.setCoeffs(c); r_.setCoeffs(c); }
    void reset() { l_.reset(); r_.reset(); }
    void process(float* interleaved, int frames) {
        for(int i=0;i<frames;i++){
            interleaved[i*2]     = l_.process(interleaved[i*2]);
            interleaved[i*2 + 1] = r_.process(interleaved[i*2 + 1]);
        }
    }
private:
    Biquad l_, r_;
};

} // namespace fury::dsp
