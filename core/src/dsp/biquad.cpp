#include "dsp/biquad.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace fury::dsp {

BiquadCoeffs Biquad::makeLowShelf(double Fs, double Fc, double gainDb, double S) {
    double A = std::pow(10.0, gainDb / 40.0);
    double w0 = 2 * M_PI * Fc / Fs;
    double cosw0 = std::cos(w0);
    double sinw0 = std::sin(w0);
    double alpha = sinw0 / 2 * std::sqrt((A + 1/A)*(1/S - 1) + 2);
    double twoSqrtA_alpha = 2 * std::sqrt(A) * alpha;

    BiquadCoeffs c;
    c.b0 = (float)( A*( (A+1) - (A-1)*cosw0 + twoSqrtA_alpha) );
    c.b1 = (float)( 2*A*( (A-1) - (A+1)*cosw0) );
    c.b2 = (float)( A*( (A+1) - (A-1)*cosw0 - twoSqrtA_alpha) );
    float a0 = (float)( (A+1) + (A-1)*cosw0 + twoSqrtA_alpha);
    c.a1 = (float)( -2*( (A-1) + (A+1)*cosw0) ) / a0;
    c.a2 = (float)( (A+1) + (A-1)*cosw0 - twoSqrtA_alpha) / a0;
    c.b0 /= a0; c.b1 /= a0; c.b2 /= a0;
    return c;
}

BiquadCoeffs Biquad::makeHighShelf(double Fs, double Fc, double gainDb, double S) {
    double A = std::pow(10.0, gainDb / 40.0);
    double w0 = 2 * M_PI * Fc / Fs;
    double cosw0 = std::cos(w0);
    double sinw0 = std::sin(w0);
    double alpha = sinw0 / 2 * std::sqrt((A + 1/A)*(1/S - 1) + 2);
    double twoSqrtA_alpha = 2 * std::sqrt(A) * alpha;

    BiquadCoeffs c;
    c.b0 = (float)( A*( (A+1) + (A-1)*cosw0 + twoSqrtA_alpha) );
    c.b1 = (float)( -2*A*( (A-1) + (A+1)*cosw0) );
    c.b2 = (float)( A*( (A+1) + (A-1)*cosw0 - twoSqrtA_alpha) );
    float a0 = (float)( (A+1) - (A-1)*cosw0 + twoSqrtA_alpha);
    c.a1 = (float)( 2*( (A-1) - (A+1)*cosw0) ) / a0;
    c.a2 = (float)( (A+1) - (A-1)*cosw0 - twoSqrtA_alpha) / a0;
    c.b0 /= a0; c.b1 /= a0; c.b2 /= a0;
    return c;
}

BiquadCoeffs Biquad::makePeaking(double Fs, double Fc, double gainDb, double Q) {
    double A = std::pow(10.0, gainDb / 40.0);
    double w0 = 2 * M_PI * Fc / Fs;
    double cosw0 = std::cos(w0);
    double sinw0 = std::sin(w0);
    double alpha = sinw0 / (2 * Q);

    BiquadCoeffs c;
    float a0 = (float)(1 + alpha / A);
    c.b0 = (float)(1 + alpha * A) / a0;
    c.b1 = (float)(-2 * cosw0) / a0;
    c.b2 = (float)(1 - alpha * A) / a0;
    c.a1 = (float)(-2 * cosw0) / a0;
    c.a2 = (float)(1 - alpha / A) / a0;
    return c;
}

} // namespace fury::dsp
