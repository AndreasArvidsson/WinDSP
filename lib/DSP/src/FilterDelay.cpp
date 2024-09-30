#include "FilterDelay.h"
#include "Constants.h"
#include <cmath> // lround
#include "Str.h"

using std::make_unique;

const uint32_t FilterDelay::getSampleDelay(const uint32_t sampleRate, double delay, const bool useUnitMeter) {
    // Value is in meter. Convert to milliseconds
    if (useUnitMeter) {
        delay = 1000.0 * delay / SPEED_OF_SOUND;
    }
    return (uint32_t)lround(sampleRate * delay / 1000.0);
}

FilterDelay::FilterDelay() {
    _delay = 0;
    _useUnitMeter = false;
    _size = _index = 0;
    _pBuffer = nullptr;
}

FilterDelay::FilterDelay(const uint32_t sampleRate, const double delay, const bool useUnitMeter) {
    _delay = delay;
    _useUnitMeter = useUnitMeter;
    _size = getSampleDelay(sampleRate, delay, useUnitMeter);
    _index = 0;
    _pBuffer = make_unique<double[]>(_size);
    reset();
}

const vector<string> FilterDelay::toString() const {
    return vector<string>{
        String::format(
            "Delay: %s%s",
            String::toString(_delay).c_str(),
            _useUnitMeter ? "m" : "ms"
        )
    };
}