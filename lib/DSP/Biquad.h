#pragma once
#include <vector>

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
	
	void initPEQ(const uint32_t sampleRate, const double frequency, const double q, const double gain);
	void initLinkwitzTransform(const uint32_t sampleRate, const double F0, const double Q0, const double Fp, const double Qp);
	
	const std::vector<std::vector<double>> getFrequencyResponse(const uint32_t sampleRate, const uint32_t nPoints, const double fMin, const double fMax) const;
    void printCoefficients(const bool miniDSPFormat = false) const;

	//Transposed direct form II 
	inline const double process(const double data) {
		const double out = data * b0 + z1;
		z1 = data * b1 - out * a1 + z2;
		z2 = data * b2 - out * a2;
		return out;
	}

	inline void reset() {
		z1 = z2 = 0;
	}

private:
	double b0, b1, b2, a0, a1, a2, z1, z2;

	double getOmega(const uint32_t sampleRate, const double frequency) const;
	double getAlpha(const double w0, const double q) const;
	void normalize();

};

