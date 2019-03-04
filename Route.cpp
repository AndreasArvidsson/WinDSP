#include "Route.h"

Route::Route(const size_t channelIndex) {
	_channelIndex = channelIndex;
	_valid = true;
}

Route::~Route() {
	for (Filter * const pFilter : _filters) {
		delete pFilter;
	}
}

void Route::addFilters(const std::vector<Filter*> &filters) {
	_filters.insert(_filters.end(), filters.begin(), filters.end());
}

void Route::addCondition(const Condition &condition) {
	_conditions.push_back(condition);
}

const size_t Route::getChannelIndex() const {
	return _channelIndex;
}

const bool Route::hasConditions() const {
	return _conditions.size() != 0;
}

const std::vector<Filter*>& Route::getFilters() const {
	return _filters;
}

void Route::evalConditions() {
	bool valid = true;
	for (const Condition &cond : _conditions) {
		if (!cond.eval()) {
			valid = false;
			break;
		}
	}
	_valid = valid;
}

void Route::reset() const {
	for (Filter * const pFilter : _filters) {
		pFilter->reset();
	}
}