#include "Output.h"

Output::Output() {
    _channel = Channel::CHANNEL_NULL;
    _mute = false;
    _clipping = 0.0;
    _usePostFilters = false;
}

Output::Output(const Channel channel, const bool mute) {
    _channel = channel;
    _mute = mute;
    _clipping = 0.0;
    _usePostFilters = false;
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

void Output::addPostFilters(vector<unique_ptr<Filter>>& filters) {
    for (unique_ptr<Filter>& pFilter : filters) {
        _postFilters.push_back(move(pFilter));
    }
    _usePostFilters = _postFilters.size() > 0;
}

const vector<unique_ptr<Filter>>& Output::getFilters() const {
    return _filters;
}

const vector<unique_ptr<Filter>>& Output::getPostFilters() const {
    return _postFilters;
}

void Output::reset() const {
    for (const unique_ptr<Filter>& pFilter : _filters) {
        pFilter->reset();
    }
    for (const unique_ptr<Filter>& pFilter : _postFilters) {
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