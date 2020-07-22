#pragma once
#include "FilterCompression.h"
#include "Convert.h"

FilterCompression::FilterCompression(const uint32_t sampleRate, const double threshold, const double ratio,
    const double attack, const double release, const double window) {
    _threshold = Convert::dbToLevel(threshold);
    _ratio = ratio;
    _attackCoef = exp(-1000.0 / (attack * sampleRate));
    _releaseCoef = exp(-1000.0 / (release * sampleRate));
    _windowCoef = exp(-1000.0 / (window * sampleRate));
    _squaredSum = 0;
    _envelope = 0;
}

const vector<string> FilterCompression::toString() const {
    return vector<string>{ "Compression" };
}