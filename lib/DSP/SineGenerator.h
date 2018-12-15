#pragma once
#define _USE_MATH_DEFINES
#include "math.h" //M_PI
#include <cmath> //pow

class SineGenerator {
public:

    SineGenerator(const double sampleRate, const double frequency, const double amplitudeDb = 0) {
        _phase = 0;
        _amplitude = dbToApmplitude(amplitudeDb);
        _phaseDelta = (2 * M_PI * frequency) / sampleRate;
    }

    const double next() {
        const double out = _amplitude * sin(_phase);
        _phase += _phaseDelta;
        return out;
    }

private:
    double _phase, _phaseDelta, _amplitude;

    const double dbToApmplitude(const double db) const {
		return std::pow(10, db / 20);
    }

};




