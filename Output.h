#pragma once
#include <vector>
#include "Filter.h"

class Output {
public:
	std::vector<Filter*> filters;
	bool mute;

	Output(const bool mute = false) {
		this->mute = mute;
	}

	~Output() {
		for (Filter *p : filters) {
			delete p;
		}
	}

	inline const double process(double data) const {
		if (mute) {
			return 0.0;
		}
		for (size_t i = 0; i < filters.size(); ++i) {
			data = filters[i]->process(data);
		}
		return data;
	}

};