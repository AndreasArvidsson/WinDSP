#pragma once
#include <vector>

using std::vector;

class Biquad {
public:
    Biquad();

    void init(const double b0, const double b1, const double b2, const double a0, const double a1, const double a2);
    void init(const double b0, const double b1, const double b2, const double a1, const double a2);

    void initLowPass(const uint32_t sampleRate, const double frequency, const double q);
    void initLowPass(const uint32_t sampleRate, const double frequency);
    void initHighPass(const uint32_t sampleRate, const double frequency, const double q);
    void initHighPass(const uint32_t sampleRate, const double frequency);

    void initLowShelf(const uint32_t sampleRate, const double frequency, const double gain, const double q = 0.707);
    void initHighShelf(const uint32_t sampleRate, const double frequency, const double gain, const double q = 0.707);

    void initBandPass(const uint32_t sampleRate, const double frequency, const double bandwidth, const double gain = 0);
    void initNotch(const uint32_t sampleRate, const double frequency, const double bandwidth, const double gain = 0);

    void initPEQ(const uint32_t sampleRate, const double frequency, const double gain, const double q);
    void initLinkwitzTransform(const uint32_t sampleRate, const double F0, const double Q0, const double Fp, const double Qp);

    const vector<vector<double>> getFrequencyResponse(const uint32_t sampleRate, const uint32_t nPoints, const double fMin, const double fMax) const;
    void printCoefficients(const bool miniDSPFormat = false) const;

    // Transposed direct form II
    inline const double process(const double data) {
        const double out = data * _b0 + _z1;
        _z1 = data * _b1 - out * _a1 + _z2;
        _z2 = data * _b2 - out * _a2;
        return out;
    }

    inline void reset() {
        _z1 = _z2 = 0;
    }

private:
    double _b0, _b1, _b2, _a0, _a1, _a2, _z1, _z2;

    double getOmega(const uint32_t sampleRate, const double frequency) const;
    double getAlpha(const double w0, const double q) const;
    void normalize();

};

