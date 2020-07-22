/*
    This class represents a single output on the render device.
    Contains all forks specified in the configuration.

    Author: Andreas Arvidsson
    Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#define NOMINMAX
#include <algorithm> //max
#include <memory>
#include "Filter.h"
#include "Channel.h"

using std::unique_ptr;
using std::max;

class Output {
public:

    Output();
    Output(const Channel channel, const bool mute = false);

    void addFilters(vector<unique_ptr<Filter>>& filters);
    void addFilter(unique_ptr<Filter> pFilter);
    void addFilterFirst(unique_ptr<Filter> pFilter);
    void addPostFilters(vector<unique_ptr<Filter>>& filters);
    const vector<unique_ptr<Filter>>& getFilters() const;
    const vector<unique_ptr<Filter>>& getPostFilters() const;
    const Channel Output::getChannel() const;
    const bool isDefined() const;
    const bool isMuted() const;
    void reset() const;
    const double resetClipping();

    inline const double process(double data) {
        if (_mute) {
            return 0.0;
        }
        for (const unique_ptr<Filter>& pFilter : _filters) {
            data = pFilter->process(data);
        }
        //Post filters all use the result of the normal filter instead of the previous result. Ie post filters are NOT chained.
        if (_usePostFilters) {
            const double preData = data;
            for (const unique_ptr<Filter>& pFilter : _postFilters) {
                data += pFilter->process(preData);
            }
        }
        if (abs(data) > 1.0) {
            //Record clipping level so it can be shown in error message.
            _clipping = max(_clipping, abs(data));
            //Clamp/limit to max value to avoid damaging equipment.
            return data > 0.0 ? 1.0 : -1.0;
        }
        return data;
    }

private:
    vector<unique_ptr<Filter>> _filters, _postFilters;
    Channel _channel;
    double _clipping;
    bool _mute, _usePostFilters;

};