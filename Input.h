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

	Input(const Channel channel);
	Input(const Channel channel, const Channel out);
	~Input();

	const std::vector<Route*>& getRoutes() const;
	void addRoute(Route * const pRoute);
    const Channel getChannel() const;

	void evalConditions() const;
	void reset();
	const bool resetIsPlaying();

	inline void route(const double data, double * const pRenderBuffer) {
		if (data) {
			_isPlaying = true;
		}
		for (const Route * const pRoute : _routes) {
			pRoute->process(data, pRenderBuffer);
		}
	}

private:
	std::vector<Route*> _routes;
    Channel _channel;
	bool _isPlaying;

};