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
	void resetState();

	void add(const double b0, const double b1, const double b2, const double a1, const double a2);
	void add(const double b0, const double b1, const double b2, const double a0, const double a1, const double a2);
	
	void addCrossover(const bool isLowPass, const double frequency, const uint8_t order, const std::vector<double> &qValues);
	void addCrossover(const bool isLowPass, const double frequency, const uint8_t order, const CrossoverType type);

	void addLowPass(const double frequency, uint8_t order, const std::vector<double> qValues);
	void addLowPass(const double frequency, const uint8_t order, const CrossoverType type);
	
	void addHighPass(const double frequency, uint8_t order, const std::vector<double> qValues);
	void addHighPass(const double frequency, const uint8_t order, const CrossoverType type);
	
	void addShelf(const bool isLowShelf, const double frequency, const double gain, const double slope = 1);
	void addLowShelf(const double frequency, const double gain, const double slope = 1);
	void addHighShelf(const double frequency, const double gain, const double slope = 1);
	
	void addPEQ(const double frequency, const double q, const double gain);
	void addBandPass(const double frequency, const double bandwidth, const double gain = 1.0);
	void addNotch(const double frequency, const double bandwidth);
	void addLinkwitzTransform(const double F0, const double Q0, const double Fp, const double Qp);

	inline const double process(double data) override {
		for (size_t i = 0; i < _biquads.size(); ++i) {
			data = _biquads[i].process(data);
		}
		return data;
	}

private:
	std::vector<Biquad> _biquads;
	uint32_t _sampleRate;

};
