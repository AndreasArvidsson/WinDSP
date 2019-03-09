#include "FilterCancellation.h"
#include "FilterGain.h"

FilterCancellation::FilterCancellation(const uint32_t sampleRate, const double frequency, const double gain) {
    const double delayMs = 1000.0 / frequency;
    _multiplier = -FilterGain::getMultiplier(gain);
    _pFilterDelay = new FilterDelay(sampleRate, delayMs);
}

FilterCancellation::~FilterCancellation() {
    delete _pFilterDelay;
}