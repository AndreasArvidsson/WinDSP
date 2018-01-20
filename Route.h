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

	inline void process(double data, float *pRenderBuffer) const {
		if (_valid) {
			for (Filter *p : filters) {
				data = p->process(data);
			}
			pRenderBuffer[out] += (float)data;
		}
	}

	inline void evalConditions() {
		_valid = true;
		for (size_t i = 0; i < conditions.size(); ++i) {
			if (!conditions[i].eval()) {
				_valid = false;
				return;
			}
		}
	}

private:
	bool _valid;

};