/*
	This class represents a single input on the capture device.
	Contains all routes specified in the configuration.

	Author: Andreas Arvidsson
	Source: https://github.com/AndreasArvidsson/WinDSP
*/

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

	inline void route(const double data, double *pRenderBuffer) const {
		for (const Route *p : routes) {
			p->process(data, pRenderBuffer);
		}
	}

	inline void evalConditions() const {
		for (Route *pRoute : routes) {
			pRoute->evalConditions();
		}
	}

	inline void reset() {
		for (const Route *pRoute : routes) {
			pRoute->reset();
		}
	}

};