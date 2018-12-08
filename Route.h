/*
	This class represents a single route on an inut.
	Contains all filters and conditions specified in the configuration.

	Author: Andreas Arvidsson
	Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <vector>
#include "Filter.h"
#include "Condition.h"

class Route {
public:
	size_t out;
	std::vector<Filter*> filters;
	std::vector<Condition> conditions;

	Route(const size_t out) {
		this->out = out;
		_valid = true;
	}

	~Route() {
		for (Filter *p : filters) {
			delete p;
		}
	}

	inline void process(double data, double *pRenderBuffer) const {
		if (_valid) {
			for (Filter *p : filters) {
				data = p->process(data);
			}
			pRenderBuffer[out] += data;
		}
	}

	inline void evalConditions() {
		bool valid = true;
		for (const Condition &cond : conditions) {
			if (!cond.eval()) {
				valid = false;
				break;
			}
		}
		_valid = valid;
	}

	inline void reset() const {
		for (Filter *p : filters) {
			 p->reset();
		}
	}

private:
	bool _valid;

};