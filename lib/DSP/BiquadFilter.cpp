#include "BiquadFilter.h"
#include "Biquad.h"

BiquadFilter::BiquadFilter(const uint32_t sampleRate) {
	_sampleRate = sampleRate;
}

void BiquadFilter::reset() {
	for (Biquad &biquad : _biquads) {
		biquad.reset();
	}
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

void BiquadFilter::addCrossover(const bool isLowPass, const double frequency, const std::vector<double> &qValues) {
	if (isLowPass) {
		addLowPass(frequency, qValues);
	}
	else {
		addHighPass(frequency, qValues);
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

void BiquadFilter::addLowPass(const double frequency, const std::vector<double> qValues) {
	for (double q : qValues) {
		Biquad biquad;
		//First order biquad
		if (q < 0) {
			biquad.initLowPass(_sampleRate, frequency);
		}
		//Second order biquad
		else {
			biquad.initLowPass(_sampleRate, frequency, q);
		}
		_biquads.push_back(biquad);
	}
}

void BiquadFilter::addLowPass(const double frequency, const uint8_t order, const CrossoverType type) {
	addLowPass(frequency, CrossoverTypes::getQValues(type, order));
}

void BiquadFilter::addHighPass(const double frequency, const std::vector<double> qValues) {
	for (double q : qValues) {
		Biquad biquad;
		if (q < 0) {
			biquad.initHighPass(_sampleRate, frequency);
		}
		else {
			biquad.initHighPass(_sampleRate, frequency, q);
		}
		_biquads.push_back(biquad);
	}
}

void BiquadFilter::addHighPass(const double frequency, const uint8_t order, const CrossoverType type) {
	addHighPass(frequency, CrossoverTypes::getQValues(type, order));
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

void BiquadFilter::printCoefficients(const bool miniDSPFormat) const {
	int index = 1;
	for (const Biquad &biquad : _biquads) {
		printf("biquad%d,\n", index++);
		biquad.printCoefficients(miniDSPFormat);
	}
	printf("\n");
}

const std::vector<std::vector<double>> BiquadFilter::getFrequencyResponse(const uint32_t nPoints, const double fMin, const double fMax) const {
	std::vector<std::vector<double>> result(nPoints);
	for (const Biquad &biquad : _biquads) {
		const std::vector<std::vector<double>> data = biquad.getFrequencyResponse(_sampleRate, nPoints, fMin, fMax);
		for (uint32_t i = 0; i < nPoints; ++i) {
			//First to for this index/Hz. Use entire pair
			if (result[i].size() == 0) {
				result[i] = data[i];
			}
			//Index/Hz already exists. Just add level/dB.
			else {
				result[i][1] += data[i][1];
			}
		}
	}
	return result;
}