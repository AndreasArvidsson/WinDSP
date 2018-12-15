#pragma once
#define _USE_MATH_DEFINES
#include "math.h" //M_PI
#include <cmath> //pow

class SineSweepGenerator {
public:

    SineSweepGenerator(const double sampleRate, const double frequencyStart, const double frequencyEnd, const double duration, const double amplitudeDb = 0) {
        _sampleRate = sampleRate;
		_frequencyStart = frequencyStart;
        _amplitude = dbToApmplitude(amplitudeDb);
        _numSamples = (size_t)(sampleRate * duration);
		_freqDelta = (frequencyEnd - frequencyStart) / (sampleRate * duration);
		reset();
    }

	const double getFrequency() const {
		return _frequency;
	}

    const size_t getNumSamples() const {
        return _numSamples;
    }
    
    const double next() {
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

	void reset() {
		_frequency = _frequencyStart;
		_phase = 0.0;
		_index = 0;
		updatePhaseDelta();
	}

private:
    double _sampleRate, _phase, _frequencyStart, _frequency, _phaseDelta, _freqDelta, _amplitude;
	size_t _index, _numSamples;

	const double dbToApmplitude(const double db) const {
		return std::pow(10, db / 20);
	}

    const void updatePhaseDelta() {
        _phaseDelta = (2 * M_PI * _frequency) / _sampleRate;
    }

};