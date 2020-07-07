/*
	This class represents a single output on the render device.
	Contains all forks specified in the configuration.

	Author: Andreas Arvidsson
	Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <vector>
#include <algorithm> //std::max
#include "Filter.h"
#include "Channel.h"

class Output {
public:

	Output(const Channel channel, const bool mute = false);
	~Output();

	void addFilters(const std::vector<Filter*> &filters);
    void addFilter(Filter *pFilter);
	void addPostFilters(const std::vector<Filter*> &filters);
	const std::vector<Filter*>& getFilters() const;
    const std::vector<Filter*>& getPostFilters() const;
    const Channel Output::getChannel() const;

	void reset();
	const double resetClipping();

	inline const double process(double data) {
		if (_mute) {
			return 0.0;
		}
		for (Filter * const pFilter : _filters) {
			data = pFilter->process(data);
		}
		//Post filters all use the result of the normal filter instead of the previous result. Ie post filters are NOT chained.
		if (_usePostFilters) {
			const double preData = data;
			for (Filter * const pFilter : _postFilters) {
				data += pFilter->process(preData);
			}
		}
		if (std::abs(data) > 1.0) {
			//Record clipping level so it can be shown in error message.
			_clipping = std::max(_clipping, std::abs(data));
			//Clamp/limit to max value to avoid damaging equipment.
			return data > 0.0 ? 1.0 : -1.0;
		}
		return data;
	}

private:
	std::vector<Filter*> _filters, _postFilters;
    Channel _channel;
	double _clipping;
	bool _mute, _usePostFilters;

};