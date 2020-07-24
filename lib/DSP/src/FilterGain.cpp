#include "FilterGain.h"
#include <cmath> //pow
#include "Str.h"

using std::pow;

const double FilterGain::getMultiplier(const double gain) {
    return pow(10.0, gain / 20.0);
}

FilterGain::FilterGain(const double gain) {
    init(gain, false);
}

FilterGain::FilterGain(const double gain, const bool invert) {
    init(gain, invert);
}

const double FilterGain::getGain() const {
    return _gain;
}

const double FilterGain::getMultiplier() const {
    return _multiplier;
}

const double FilterGain::getMultiplierNoInvert() const {
    return _multiplierNoInvert;
}

const bool FilterGain::getInvert() const {
    return _invert;
}

void FilterGain::setGain(const double gain) {
    _gain = gain;
    _multiplierNoInvert = getMultiplier(gain);
    _multiplier = _invert ? _multiplierNoInvert * -1.0 : _multiplierNoInvert;
}

void FilterGain::init(const double gain, const bool invert) {
    _invert = invert;
    setGain(gain);
}

const vector<string> FilterGain::toString() const {
    vector<string> res{
       String::format("Gain: %sdB", String::formatFloat(_gain).c_str())
    };
    if (_invert) {
        res.push_back("Inverted");
    }
    return res;
}