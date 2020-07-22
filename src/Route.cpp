#include "Route.h"

Route::Route() {
    _channel = Channel::CHANNEL_NULL;
    _channelIndex = (size_t)-1;
    _valid = true;
}

Route::Route(const Channel channel) {
    _channel = channel;
    _channelIndex = (size_t)channel;
    _valid = true;
}

void Route::addFilters(vector<unique_ptr<Filter>>& filters) {
    for (unique_ptr<Filter>& pFilter : filters) {
        _filters.push_back(move(pFilter));
    }
}

void Route::addFilter(unique_ptr<Filter> pFilter) {
    _filters.push_back(move(pFilter));
}

void Route::addCondition(const Condition& condition) {
    _conditions.push_back(condition);
}

const Channel Route::getChannel() const {
    return _channel;
}

const size_t Route::getChannelIndex() const {
    return _channelIndex;
}

const bool Route::hasConditions() const {
    return _conditions.size() != 0;
}

const vector<unique_ptr<Filter>>& Route::getFilters() const {
    return _filters;
}

void Route::evalConditions() {
    bool valid = true;
    for (const Condition& cond : _conditions) {
        if (!cond.eval()) {
            valid = false;
            break;
        }
    }
    _valid = valid;
}

void Route::reset() const {
    for (const unique_ptr<Filter>& pFilter : _filters) {
        pFilter->reset();
    }
}