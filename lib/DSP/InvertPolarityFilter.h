#pragma once
#include "Filter.h"

class InvertPolarityFilter : public Filter {
public:

	inline const double process(double value) override {
		return value * -1.0;
	}

};
