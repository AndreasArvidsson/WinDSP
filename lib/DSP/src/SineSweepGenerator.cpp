#include "SineSweepGenerator.h"
#define _USE_MATH_DEFINES
#include "math.h" // M_PI
#include "Convert.h"

SineSweepGenerator::SineSweepGenerator(const size_t sampleRate, const double frequencyStart, const double frequencyEnd, const double duration, const double gain) {
    _sampleRate = sampleRate;
    _frequencyStart = frequencyStart;
    _amplitude = Convert::dbToLevel(gain);
    _numSamples = (size_t)(sampleRate * duration);
    _freqDelta = (frequencyEnd - frequencyStart) / (sampleRate * duration);
    reset();
}

const double SineSweepGenerator::getFrequency() const {
    return _frequency;
}

const size_t SineSweepGenerator::getNumSamples() const {
    return _numSamples;
}

const double SineSweepGenerator::next() {
    const double out = _amplitude * sin(_phase);
    ++_index;
    if (_index == _numSamples) {
        reset();
    }
    else {
        _phase += _phaseDelta;
        _frequency += _freqDelta;
        updatePhaseDelta();
    }
    return out;
}

void SineSweepGenerator::reset() {
    _frequency = _frequencyStart;
    _phase = 0.0;
    _index = 0;
    updatePhaseDelta();
}

const void SineSweepGenerator::updatePhaseDelta() {
    _phaseDelta = (2 * M_PI * _frequency) / _sampleRate;
}