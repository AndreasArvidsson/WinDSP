/*
    This class represents a single route on an inut.
    Contains all filters and conditions specified in the configuration.

    Author: Andreas Arvidsson
    Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <vector>
#include <memory>
#include "Filter.h"
#include "Condition.h"
#include "Channel.h"

using std::vector;
using std::unique_ptr;

class Route {
public:

    Route();
    Route(const Channel channel);

    void addFilters(vector<unique_ptr<Filter>>& filters);
    void addFilter(unique_ptr<Filter> pFilter);
    void addCondition(const Condition& condition);
    const Channel getChannel() const;
    const size_t getChannelIndex() const;
    const bool hasConditions() const;
    const vector<unique_ptr<Filter>>& getFilters() const;
    void evalConditions();
    void reset() const;

    inline void process(double data, double* const pRenderBuffer) const {
        if (_valid) {
            for (const unique_ptr<Filter>& pFilter : _filters) {
                data = pFilter->process(data);
            }
            pRenderBuffer[_channelIndex] += data;
        }
    }

private:
    vector<unique_ptr<Filter>> _filters;
    vector<Condition> _conditions;
    Channel _channel;
    size_t _channelIndex;
    bool _valid;

};