#pragma once
#include "Filter.h"

class GainFilter : public Filter {
public:

	GainFilter(const double gain) {
		_gain = gain;
		_multiplier = std::pow(10, gain / 20);
	}

	inline const double process(const double value) override {
		return _multiplier * value;
	}

	const double getGain() const {
		return _gain;
	}

	const double getMultiplier() const {
		return _multiplier;
	}

	inline void reset() override {
	}

private:
	double _gain, _multiplier;

};
