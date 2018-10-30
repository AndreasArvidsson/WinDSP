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

	Input(const std::string &name) {
		_name = name;
		_isPlaying = false;
	}

	Input(const std::string &name, const size_t out) {
		_name = name;
		_isPlaying = false;
		routes.push_back(new Route(out));
	}

	~Input() {
		for (Route *p : routes) {
			delete p;
		}
	}

	inline void route(const double data, double *pRenderBuffer) {
		if (data != 0.0) {
			_isPlaying = true;
		}
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

	const std::string getName() const {
		return _name;
	}

	const bool resetIsPlaying() {
		if (_isPlaying) {
			_isPlaying = false;
			return true;
		}
		return false;
	}

private:
	std::string _name;
	bool _isPlaying;

};