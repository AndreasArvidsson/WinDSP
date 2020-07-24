#pragma once
#include "FilterCompression.h"
#include "Convert.h"

FilterCompression::FilterCompression(const uint32_t sampleRate, const double threshold, const double ratio,
    const double attack, const double release, const double window) {
    _threshold = Convert::dbToLevel(threshold);
    _ratio = ratio - 1;
    _squaredSum = 0;
    _envelope = 0;
    _attackCoef = exp(-1000.0 / (attack * sampleRate));
    _releaseCoef = exp(-1000.0 / (release * sampleRate));
    if (window > 0) {
        _useWindow = true;
        _windowCoef = exp(-1000.0 / (window * sampleRate));
    }
    else {
        _useWindow = false;
        _windowCoef = 0;
    }
}

const vector<string> FilterCompression::toString() const {
    return vector<string>{ "Compression" };
}