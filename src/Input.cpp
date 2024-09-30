#include "Input.h"

using std::move;

Input::Input() {
    _channel = Channel::CHANNEL_NULL;
    _isPlaying = false;
}

Input::Input(const Channel channel) {
    _channel = channel;
    _isPlaying = false;
}

Input::Input(const Channel channel, const Channel out) {
    _channel = channel;
    _isPlaying = false;
    _routes.push_back(Route(out));
}

const vector<Route>& Input::getRoutes() const {
    return _routes;
}

void Input::addRoute(Route& route) {
    _routes.push_back(move(route));
}

const Channel Input::getChannel() const {
    return _channel;
}

const bool Input::isDefined() const {
    return _channel != Channel::CHANNEL_NULL;
}

void Input::evalConditions() {
    for (Route& route : _routes) {
        route.evalConditions();
    }
}

void Input::reset() {
    for (const Route& route : _routes) {
        route.reset();
    }
}

const bool Input::resetIsPlaying() {
    if (_isPlaying) {
        _isPlaying = false;
        return true;
    }
    return false;
}