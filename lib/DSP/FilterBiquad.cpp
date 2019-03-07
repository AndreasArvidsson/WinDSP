#include "FilterBiquad.h"
#include "Biquad.h"

FilterBiquad::FilterBiquad(const uint32_t sampleRate) {
	_sampleRate = sampleRate;
}

const size_t FilterBiquad::size() const {
	return _biquads.size();
}

const bool FilterBiquad::isEmpty() const {
	return _biquads.size() == 0;
}

const uint32_t FilterBiquad::getSampleRate() const {
	return _sampleRate;
}

void FilterBiquad::add(const double b0, const double b1, const double b2, const double a1, const double a2) {
	Biquad biquad;
	biquad.init(b0, b1, b2, a1, a2);
	_biquads.push_back(biquad);
}

void FilterBiquad::add(const double b0, const double b1, const double b2, const double a0, const double a1, const double a2) {
	Biquad biquad;
	biquad.init(b0, b1, b2, a0, a1, a2);
	_biquads.push_back(biquad);
}

void FilterBiquad::addCrossover(const bool isLowPass, const double frequency, const std::vector<double> &qValues) {
	if (isLowPass) {
		addLowPass(frequency, qValues);
	}
	else {
		addHighPass(frequency, qValues);
	}
}

void FilterBiquad::addCrossover(const bool isLowPass, const double frequency, const uint8_t order, const CrossoverType type) {
	if (isLowPass) {
		addLowPass(frequency, order, type);
	}
	else {
		addHighPass(frequency, order, type);
	}
}

void FilterBiquad::addLowPass(const double frequency, const std::vector<double> qValues) {
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

void FilterBiquad::addLowPass(const double frequency, const uint8_t order, const CrossoverType type) {
	addLowPass(frequency, CrossoverTypes::getQValues(type, order));
}

void FilterBiquad::addHighPass(const double frequency, const std::vector<double> qValues) {
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

void FilterBiquad::addHighPass(const double frequency, const uint8_t order, const CrossoverType type) {
	addHighPass(frequency, CrossoverTypes::getQValues(type, order));
}

void FilterBiquad::addShelf(const bool isLowShelf, const double frequency, const double gain, const double q) {
	if (isLowShelf) {
		addLowShelf(frequency, gain, q);
	}
	else {
		addHighShelf(frequency, gain, q);
	}
}

void FilterBiquad::addLowShelf(const double frequency, const double gain, const double q) {
	Biquad biquad;
	biquad.initLowShelf(_sampleRate, frequency, gain, q);
	_biquads.push_back(biquad);
}

void FilterBiquad::addHighShelf(const double frequency, const double gain, const double q) {
	Biquad biquad;
	biquad.initHighShelf(_sampleRate, frequency,  gain, q);
	_biquads.push_back(biquad);
}

void FilterBiquad::addPEQ(const double frequency, const double q, const double gain) {
	Biquad biquad;
	biquad.initPEQ(_sampleRate, frequency, q, gain);
	_biquads.push_back(biquad);
}

void FilterBiquad::addBandPass(const double frequency, const double bandwidth, const double gain) {
	Biquad biquad;
	biquad.initBandPass(_sampleRate, frequency, bandwidth, gain);
	_biquads.push_back(biquad);
}

void FilterBiquad::addNotch(const double frequency, const double bandwidth, const double gain) {
	Biquad biquad;
	biquad.initNotch(_sampleRate, frequency, bandwidth, gain);
	_biquads.push_back(biquad);
}

void FilterBiquad::addLinkwitzTransform(const double f0, const double q0, const double fp, const double qp) {
	Biquad biquad;
	biquad.initLinkwitzTransform(_sampleRate, f0, q0, fp, qp);
	_biquads.push_back(biquad);
}

void FilterBiquad::printCoefficients(const bool miniDSPFormat) const {
	int index = 1;
	for (const Biquad &biquad : _biquads) {
		printf("biquad%d,\n", index++);
		biquad.printCoefficients(miniDSPFormat);
	}
	printf("\n");
}

const std::vector<std::vector<double>> FilterBiquad::getFrequencyResponse(const uint32_t nPoints, const double fMin, const double fMax) const {
	std::vector<std::vector<double>> result(nPoints);
	for (const Biquad &biquad : _biquads) {
		const std::vector<std::vector<double>> data = biquad.getFrequencyResponse(_sampleRate, nPoints, fMin, fMax);
		for (uint32_t i = 0; i < nPoints; ++i) {
			//First use of this index/Hz. Use entire Hz/dB pair.
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