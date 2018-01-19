#pragma once
#include "Filter.h"

class GainFilter : public Filter {
public:

	GainFilter(const double gain) {
		_gain = gain;
		_scale = std::pow(10, gain / 20);
	}

	inline const double process(double value) override {
		return _scale * value;
	}

	const double getGain() const {
		return _gain;
	}

	const double getScale() const {
		return _scale;
	}

private:
	double _gain, _scale;

};
