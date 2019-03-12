#include "FilterCancellation.h"
#include "FilterGain.h"
#include "Str.h"

FilterCancellation::FilterCancellation(const uint32_t sampleRate, const double frequency, const double gain) {
    _frequency = frequency;
    _gain = gain;
    const double delayMs = 1000.0 / frequency;
    _multiplier = -FilterGain::getMultiplier(gain);
    _pFilterDelay = new FilterDelay(sampleRate, delayMs);
}

FilterCancellation::~FilterCancellation() {
    delete _pFilterDelay;
}

const std::string FilterCancellation::toString() const {
    return String::format("Cancel: %0.fHz", _frequency);
}