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
#include "Channel.h"

class Route {
public:

	Route(const Channel channel);
	~Route();

	void addFilters(const std::vector<Filter*> &filters);
    void addFilter(Filter *pFilter);
	void addCondition(const Condition &condition);
    const Channel getChannel() const;
	const size_t getChannelIndex() const;
	const bool hasConditions() const;
	const std::vector<Filter*>& getFilters() const;

	void evalConditions();
	void reset() const;

	inline void process(double data, double * const pRenderBuffer) const {
		if (_valid) {
			for (Filter * const pFilter : _filters) {
				data = pFilter->process(data);
			}
			pRenderBuffer[_channelIndex] += data;
		}
	}

private:
	std::vector<Filter*> _filters;
	std::vector<Condition> _conditions;
	bool _valid;
    Channel _channel;
	size_t _channelIndex;
	
};