#pragma once
#include "DelayFilter.h"

class CancellationFilter : public Filter {
public:

	CancellationFilter(const uint32_t sampleRate, double delay, const bool useUnitMeter = false) {
		_delayFilter.init(DelayFilter::getSampleDelay(sampleRate, delay, useUnitMeter));
	}

	inline const double process(double data) override {
		return _delayFilter.process(data) * -1.0;
	}

	inline void reset() override {
		_delayFilter.reset();
	}

private:
	DelayFilter _delayFilter;

};
