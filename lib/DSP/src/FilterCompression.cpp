#pragma once
#include "FilterCompression.h"
#include "Convert.h"
#include "Str.h"

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
    _toStringValue = getToStringValue(threshold, ratio, attack, release, window);
}

const vector<string> FilterCompression::toString() const {
    return vector<string>{ _toStringValue  };
}

const string FilterCompression::getToStringValue(
    const double threshold,
    const double ratio,
    const double attack,
    const double release,
    const double window) {
    return String::format(
        "Compression: threshold %sdB, ratio %s, attack %sms, release %sms, window %sms",
        String::formatFloat(threshold).c_str(),
        String::formatFloat(ratio).c_str(),
        String::formatFloat(attack).c_str(),
        String::formatFloat(release).c_str(),
        String::formatFloat(window).c_str()
    );
}