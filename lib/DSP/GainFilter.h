#pragma once
#include "Filter.h"

class GainFilter : public Filter {
public:

	GainFilter(const double gain) {
		init(gain, false);
	}

	GainFilter(const double gain, const bool invert) {
		init(gain, invert);
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

	const double getMultiplierNoInvert() const {
		return _multiplierNoInvert;
	}

	const bool getInvert() const {
		return _invert;
	}

	inline void reset() override {
	}

private:
	double _gain, _multiplier, _multiplierNoInvert;
	bool _invert;

	void init(const double gain, const bool invert) {
		_gain = gain;
		_invert = invert;
		_multiplierNoInvert = std::pow(10.0, gain / 20.0);
		_multiplier = invert ? _multiplierNoInvert * -1.0 : _multiplierNoInvert;
	}

};
