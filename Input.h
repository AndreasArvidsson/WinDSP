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

	Input(const std::string &name);
	Input(const std::string &name, const size_t out);
	~Input();

	const std::vector<Route*>& getRoutes() const;
	void addRoute(Route * const pRoute);
	const std::string getName() const;

	void evalConditions() const;
	void reset();
	const bool resetIsPlaying();

	inline void route(const double data, double * const pRenderBuffer) {
		if (data != 0.0) {
			_isPlaying = true;
		}
		for (const Route * const pRoute : _routes) {
			pRoute->process(data, pRenderBuffer);
		}
	}

private:
	std::vector<Route*> _routes;
	std::string _name;
	bool _isPlaying;

};