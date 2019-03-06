#pragma once
#include <vector>
#include "Filter.h"
#include "Biquad.h"
#include "CrossoverTypes.h"

class BiquadFilter : public Filter {
public:

	BiquadFilter(const uint32_t sampleRate);

	const size_t size() const;
	const bool isEmpty() const;
	const uint32_t getSampleRate() const;

	void add(const double b0, const double b1, const double b2, const double a1, const double a2);
	void add(const double b0, const double b1, const double b2, const double a0, const double a1, const double a2);
	
	void addCrossover(const bool isLowPass, const double frequency, const std::vector<double> &qValues);
	void addCrossover(const bool isLowPass, const double frequency, const uint8_t order, const CrossoverType type);

	void addLowPass(const double frequency, const std::vector<double> qValues);
	void addLowPass(const double frequency, const uint8_t order, const CrossoverType type);
	
	void addHighPass(const double frequency, const std::vector<double> qValues);
	void addHighPass(const double frequency, const uint8_t order, const CrossoverType type);
	
	void addShelf(const bool isLowShelf, const double frequency, const double gain, const double q = 0.707);
	void addLowShelf(const double frequency, const double gain, const double q = 0.707);
	void addHighShelf(const double frequency, const double gain, const double q = 0.707);
	
	void addBandPass(const double frequency, const double bandwidth, const double gain = 0);
	void addNotch(const double frequency, const double bandwidth, const double gain = 0);

	void addPEQ(const double frequency, const double q, const double gain);
	void addLinkwitzTransform(const double f0, const double q0, const double fp, const double qp);
	
	void printCoefficients(const bool miniDSPFormat = false) const;
	const std::vector<std::vector<double>> getFrequencyResponse(const uint32_t nPoints, const double fMin, const double fMax) const;

	inline const double process(double data) override {
		for (Biquad &biquad : _biquads) {
			data = biquad.process(data);
		}
		return data;
	}

	inline void reset() override {
		for (Biquad &biquad : _biquads) {
			biquad.reset();
		}
	}

private:
	std::vector<Biquad> _biquads;
	uint32_t _sampleRate;

};
