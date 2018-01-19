#pragma once
#include <vector>
#include "Filter.h"

class Route {
public:
	size_t out;
	std::vector<Filter*> filters;

	Route(const size_t out) : out(out) {}
	
	~Route() {
		for (Filter *p : filters) {
			delete p;
		}
	}

	inline void process(double data, float *pRenderBuffer) const {
		for (Filter *p : filters) {
			data = p->process(data);
		}
		pRenderBuffer[out] += (float)data;
	}

};