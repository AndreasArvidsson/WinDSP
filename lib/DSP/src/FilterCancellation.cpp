#include "FilterCancellation.h"
#include "FilterGain.h"
#include "Str.h"

using std::make_unique;

FilterCancellation::FilterCancellation(const uint32_t sampleRate, const double frequency, const double gain) {
    _frequency = frequency;
    _gain = gain;
    const double delayMs = 1000.0 / frequency;
    _multiplier = -FilterGain::getMultiplier(gain);
    _pFilterDelay = make_unique<FilterDelay>(sampleRate, delayMs);
}

const vector<string> FilterCancellation::toString() const {
    return vector<string>{
        String::format(
            "Cancel: freq %sHz, gain %sdB",
            String::toString(_frequency).c_str(),
            String::toString(_gain).c_str()
        )
    };
}