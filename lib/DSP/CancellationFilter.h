#pragma once
#include "DelayFilter.h"

class CancellationFilter : public Filter {
public:

	CancellationFilter(const uint32_t sampleRate, const double frequency) {
		const double delayMs = 1000 / frequency;
		_delayFilter.init(DelayFilter::getSampleDelay(sampleRate, delayMs));
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
