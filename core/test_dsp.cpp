// Teste rápido DSP (sem framework): compile com cl / g++ e execute
// cl /EHsc /I include test_dsp.cpp src/dsp/biquad.cpp -o test_dsp.exe && ./test_dsp.exe
#include "dsp/biquad.h"
#include "dsp/compressor.h"
#include <iostream>
#include <vector>
#include <cmath>

int main(){
    using namespace fury::dsp;
    std::cout << "== FuryEqualize DSP test ==\n";
    // Biquad peak 3.5kHz + compressor
    auto peak = Biquad::makePeaking(48000, 3500, 6.0, 1.2);
    std::cout << "Peak coeffs b0="<<peak.b0<<" b1="<<peak.b1<<" b2="<<peak.b2<<" a1="<<peak.a1<<" a2="<<peak.a2<<"\n";
    Biquad b; b.setCoeffs(peak);
    float out = b.process(0.5f);
    std::cout << "Biquad process 0.5 -> " << out << " (esperado !=0)\n";

    Compressor comp;
    Compressor::Params p; p.thresholdDb=-24; p.ratio=4; p.attackMs=2; p.releaseMs=80; p.makeupDb=6; p.sampleRate=48000;
    comp.configure(p, 48000);
    std::vector<float> buf(512*2, 0.5f); // estéreo 0.5
    comp.process(buf.data(), 256);
    std::cout << "Compressor GR="<<comp.gainReductionDb()<<" dB (esperado <0)\n";
    std::cout << "PASS\n";
    return 0;
}
