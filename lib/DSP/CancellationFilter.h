#pragma once
#include "DelayFilter.h"

class CancellationFilter : public Filter {
public:

	CancellationFilter(const uint32_t sampleRate, const double frequency, const double gain = 0.0) {
		const double delayMs = 1000.0 / frequency;
		_multiplier = -GainFilter::getMultiplier(gain);
		_delayFilter.init(DelayFilter::getSampleDelay(sampleRate, delayMs));
	}

	inline const double process(double data) override {
		return _delayFilter.process(data) * _multiplier;
	}

	inline void reset() override {
		_delayFilter.reset();
	}

private:
	DelayFilter _delayFilter;
	double _multiplier;

};
