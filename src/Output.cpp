#include "Output.h"

Output::Output() {
    _channel = Channel::CHANNEL_NULL;
    _mute = false;
    _clipping = 0.0;
}

Output::Output(const Channel channel, const bool mute) {
    _channel = channel;
    _mute = mute;
    _clipping = 0.0;
}

void Output::addFilters(vector<unique_ptr<Filter>>& filters) {
    for (unique_ptr<Filter>& pFilter : filters) {
        _filters.push_back(move(pFilter));
    }
}

void Output::addFilter(unique_ptr<Filter> pFilter) {
    _filters.push_back(move(pFilter));
}

void Output::addFilterFirst(unique_ptr<Filter> pFilter) {
    _filters.insert(_filters.begin(), move(pFilter));
}

const vector<unique_ptr<Filter>>& Output::getFilters() const {
    return _filters;
}

void Output::reset() const {
    for (const unique_ptr<Filter>& pFilter : _filters) {
        pFilter->reset();
    }
}

const Channel Output::getChannel() const {
    return _channel;
}

const bool Output::isDefined() const {
    return _channel != Channel::CHANNEL_NULL;
}

const bool Output::isMuted() const {
    return _mute;
}

const double Output::resetClipping() {
    const double result = _clipping;
    _clipping = 0.0;
    return result;
}