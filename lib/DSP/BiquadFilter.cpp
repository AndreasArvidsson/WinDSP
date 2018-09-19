#include "BiquadFilter.h"
#include "Biquad.h"

BiquadFilter::BiquadFilter(const uint32_t sampleRate) {
	_sampleRate = sampleRate;
}

const size_t BiquadFilter::size() const {
	return _biquads.size();
}

const bool BiquadFilter::isEmpty() const {
	return _biquads.size() == 0;
}

void BiquadFilter::add(const double b0, const double b1, const double b2, const double a1, const double a2) {
	Biquad biquad;
	biquad.init(b0, b1, b2, a1, a2);
	_biquads.push_back(biquad);
}

void BiquadFilter::add(const double b0, const double b1, const double b2, const double a0, const double a1, const double a2) {
	Biquad biquad;
	biquad.init(b0, b1, b2, a0, a1, a2);
	_biquads.push_back(biquad);
}

void BiquadFilter::addCrossover(const bool isLowPass, const double frequency, const uint8_t order, const std::vector<double> &qValues) {
	if (isLowPass) {
		addLowPass(frequency, order, qValues);
	}
	else {
		addHighPass(frequency, order, qValues);
	}
}

void BiquadFilter::addCrossover(const bool isLowPass, const double frequency, const uint8_t order, const CrossoverType type) {
	if (isLowPass) {
		addLowPass(frequency, order, type);
	}
	else {
		addHighPass(frequency, order, type);
	}
}

void BiquadFilter::addLowPass(const double frequency, uint8_t order, const std::vector<double> qValues) {
	for (int i = 0; order > 0; ++i) {
		Biquad biquad;
		//First order biquad
		if (qValues[i] < 0) {
			biquad.initLowPass(_sampleRate, frequency);
			order -= 1;
		}
		//Second order biquad
		else {
			biquad.initLowPass(_sampleRate, frequency, qValues[i]);
			order -= 2;
		}
		_biquads.push_back(biquad);
	}
}

void BiquadFilter::addLowPass(const double frequency, const uint8_t order, const CrossoverType type) {
	addLowPass(frequency, order, CrossoverTypes::getQValues(type, order));
}

void BiquadFilter::addHighPass(const double frequency, uint8_t order, const std::vector<double> qValues) {
	for (int i = 0; order > 0; ++i) {
		Biquad biquad;
		if (qValues[i] < 0) {
			biquad.initHighPass(_sampleRate, frequency);
			order -= 1;
		}
		else {
			biquad.initHighPass(_sampleRate, frequency, qValues[i]);
			order -= 2;
		}
		_biquads.push_back(biquad);
	}
}

void BiquadFilter::addHighPass(const double frequency, const uint8_t order, const CrossoverType type) {
	addHighPass(frequency, order, CrossoverTypes::getQValues(type, order));
}

void BiquadFilter::addShelf(const bool isLowShelf, const double frequency, const double gain, const double slope) {
	if (isLowShelf) {
		addLowShelf(frequency, gain, slope);
	}
	else {
		addHighShelf(frequency, gain, slope);
	}
}

void BiquadFilter::addLowShelf(const double frequency, const double gain, const double slope) {
	Biquad biquad;
	biquad.initLowShelf(_sampleRate, frequency, gain, slope);
	_biquads.push_back(biquad);
}

void BiquadFilter::addHighShelf(const double frequency, const double gain, const double slope) {
	Biquad biquad;
	biquad.initHighShelf(_sampleRate, frequency,  gain, slope);
	_biquads.push_back(biquad);
}

void BiquadFilter::addPEQ(const double frequency, const double q, const double gain) {
	Biquad biquad;
	biquad.initPEQ(_sampleRate, frequency, q, gain);
	_biquads.push_back(biquad);
}

void BiquadFilter::addBandPass(const double frequency, const double bandwidth, const double gain) {
	Biquad biquad;
	biquad.initBandPass(_sampleRate, frequency, bandwidth, gain);
	_biquads.push_back(biquad);
}

void BiquadFilter::addNotch(const double frequency, const double bandwidth) {
	Biquad biquad;
	biquad.initNotch(_sampleRate, frequency, bandwidth);
	_biquads.push_back(biquad);
}

void BiquadFilter::addLinkwitzTransform(const double F0, const double Q0, const double Fp, const double Qp) {
	Biquad biquad;
	biquad.initLinkwitzTransform(_sampleRate, F0, Q0, Fp, Qp);
	_biquads.push_back(biquad);
}