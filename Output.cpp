#include "Output.h"

Output::Output(const std::string &name, const bool mute) {
	_name = name;
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

void Output::reset() {
	for (Filter * const pFilter : _filters) {
		pFilter->reset();
	}
}

const std::string Output::getName() const {
	return _name;
}

const double Output::resetClipping() {
	const double result = _clipping;
	_clipping = 0.0;
	return result;
}