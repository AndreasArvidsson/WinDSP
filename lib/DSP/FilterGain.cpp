#include "FilterGain.h"
#include <cmath> //pow

const double FilterGain::getMultiplier(const double gain) {
    return std::pow(10.0, gain / 20.0);
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

void FilterGain::init(const double gain, const bool invert) {
    _gain = gain;
    _invert = invert;
    _multiplierNoInvert = getMultiplier(gain);
    _multiplier = invert ? _multiplierNoInvert * -1.0 : _multiplierNoInvert;
}