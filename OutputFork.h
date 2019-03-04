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

	~OutputFork() {
		for (Filter * const pFilter : _filters) {
			delete pFilter;
		}
	}

	void addFilters(const std::vector<Filter*> &filters) {
		_filters.insert(_filters.end(), filters.begin(), filters.end());
	}

	const std::vector<Filter*>& getFilters() const {
		return _filters;
	}

	inline const double process(double data) const {
		for (Filter * const pFilter : _filters) {
			data = pFilter->process(data);
		}
		return data;
	}

	void reset() const {
		for (Filter * const pFilter : _filters) {
			pFilter->reset();
		}
	}

private:
	std::vector<Filter*> _filters;

};