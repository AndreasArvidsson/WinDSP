#include "Output.h"

Output::Output(const Channel channel, const bool mute) {
	_channel = channel;
	_mute = mute;
	_clipping = 0.0;
	_usePostFilters = false;
}

Output::~Output() {
	for (Filter * const pFilter : _filters) {
		delete pFilter;
	}
}

void Output::addFilters(const std::vector<Filter*> &filters) {
	for (Filter * const pFilter : filters) {
		_filters.push_back(pFilter);
	}
}

void Output::addPostFilters(const std::vector<Filter*> &filters) {
	for (Filter * const pFilter : filters) {
		_postFilters.push_back(pFilter);
	}
	_usePostFilters = _postFilters.size() > 0;
}

const std::vector<Filter*>& Output::getFilters() const {
	return _filters;
}

const std::vector<Filter*>& Output::getPostFilters() const {
    return _postFilters;
}

void Output::reset() {
	for (Filter * const pFilter : _filters) {
		pFilter->reset();
	}
}

const Channel Output::getChannel() const {
	return _channel;
}

const double Output::resetClipping() {
	const double result = _clipping;
	_clipping = 0.0;
	return result;
}