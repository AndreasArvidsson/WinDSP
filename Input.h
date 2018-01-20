#pragma once
#include "Route.h"

class Input {
public:
	std::vector<Route*> routes;

	Input() {}

	Input(const size_t out) {
		routes.push_back(new Route(out));
	}

	~Input() {
		for (Route *p : routes) {
			delete p;
		}
	}

	inline void route(const double data, float *pRenderBuffer) const {
		for (const Route *p : routes) {
			p->process(data, pRenderBuffer);
		}
	}

	inline void evalConditions() const {
		for (Route *pRoute : routes) {
			pRoute->evalConditions();
		}
	}

};