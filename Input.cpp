#include "Input.h"

Input::Input(const std::string &name) {
	_name = name;
	_isPlaying = false;
}

Input::Input(const std::string &name, const size_t out) {
	_name = name;
	_isPlaying = false;
	_routes.push_back(new Route(out));
}

Input::~Input() {
	for (Route *pRoute : _routes) {
		delete pRoute;
	}
}

const std::vector<Route*>& Input::getRoutes() const {
	return _routes;
}

void Input::addRoute(Route * const pRoute) {
	_routes.push_back(pRoute);
}

const std::string Input::getName() const {
	return _name;
}

void Input::evalConditions() const {
	for (Route * const pRoute : _routes) {
		pRoute->evalConditions();
	}
}

void Input::reset() {
	for (const Route * const pRoute : _routes) {
		pRoute->reset();
	}
}

const bool Input::resetIsPlaying() {
	if (_isPlaying) {
		_isPlaying = false;
		return true;
	}
	return false;
}