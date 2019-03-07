#pragma once
#include "FilterDelay.h"

class FilterCancellation : public Filter {
public:

	FilterCancellation(const uint32_t sampleRate, const double frequency, const double gain = 0.0) {
		const double delayMs = 1000.0 / frequency;
		_multiplier = -FilterGain::getMultiplier(gain);
		_FilterDelay.init(FilterDelay::getSampleDelay(sampleRate, delayMs));
	}

	inline const double process(const double data) override {
		return _FilterDelay.process(data) * _multiplier;
	}

	inline void reset() override {
		_FilterDelay.reset();
	}

private:
	FilterDelay _FilterDelay;
	double _multiplier;

};
