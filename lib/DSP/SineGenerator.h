#pragma once
#define _USE_MATH_DEFINES
#include "math.h"

class SineGenerator {
public:

	SineGenerator(const size_t sampleRate, const double frequency) {
		_theta = 0;
		_sampleIncrement = (frequency * (M_PI * 2)) / (double)sampleRate;
	}

	const double next() {
		_theta += _sampleIncrement;
		return sin(_theta);
	}

private:
	double _theta;
	double _sampleIncrement;

};

