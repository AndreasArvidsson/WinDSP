/*
	This class represents a single output fork on the render device.
	Contains all filters specified in the configuration.

	Author: Andreas Arvidsson
	Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <vector>
#include "Filter.h"

class OutputFork {
public:
	std::vector<Filter*> filters;

	~OutputFork() {
		for (Filter *pFilter : filters) {
			delete pFilter;
		}
	}

	inline const double process(double data) const {
		for (Filter *pFilter : filters) {
			data = pFilter->process(data);
		}
		return data;
	}

};