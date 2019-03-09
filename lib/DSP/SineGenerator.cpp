#include "SineGenerator.h"
#include "Convert.h"
#define _USE_MATH_DEFINES
#include "math.h" //M_PI

SineGenerator::SineGenerator(const double sampleRate, const double frequency, const double gain) {
    _phase = 0;
    _amplitude = Convert::dbToLevel(gain);
    _phaseDelta = (2 * M_PI * frequency) / sampleRate;
}

const double SineGenerator::next() {
    const double out = _amplitude * sin(_phase);
    _phase += _phaseDelta;
    return out;
}