#include "FilterDelay.h"
#include "Constants.h"
#include <cmath> //lround
#include "Str.h"

const uint32_t FilterDelay::getSampleDelay(const uint32_t sampleRate, double delay, const bool useUnitMeter) {
    //Value is in meter. Convert to milliseconds
    if (useUnitMeter) {
        delay = 1000.0 * delay / SPEED_OF_SOUND;
    }
    return (uint32_t)std::lround(sampleRate * delay / 1000.0);
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
    _pBuffer = new double[_size];
    reset();
}

FilterDelay::~FilterDelay() {
    delete[] _pBuffer;
}

const std::string FilterDelay::toString() const {
    return String::format("Delay: %.1f%s", _delay, _useUnitMeter ? "m" : "ms");
}