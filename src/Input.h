/*
	This class represents a single input on the capture device.
	Contains all routes specified in the configuration.

	Author: Andreas Arvidsson
	Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include "Route.h"
#include "Channel.h"

class Input {
public:

	Input();
	Input(const Channel channel);
	Input(const Channel channel, const Channel out);

	const vector<Route>& getRoutes() const;
	void addRoute(Route& route);
    const Channel getChannel() const;
	const bool isDefined() const;

	void evalConditions();
	void reset();
	const bool resetIsPlaying();

	inline void route(const double data, double * const pRenderBuffer) {
		if (data) {
			_isPlaying = true;
		}
		for (const Route& route : _routes) {
			route.process(data, pRenderBuffer);
		}
	}

private:
	vector<Route> _routes;
    Channel _channel;
	bool _isPlaying;

};