#pragma once
#include "Filter.h"

class InvertPolarityFilter : public Filter {
public:

	inline const double process(const double value) override {
		return value * -1.0;
	}

	inline void reset() override {
	}

};
