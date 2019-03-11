#include "FilterDelay.h"
#include "Constants.h"
#include <cmath> //lround

const uint32_t FilterDelay::getSampleDelay(const uint32_t sampleRate, double delay, const bool useUnitMeter) {
    //Value is in meter. Convert to milliseconds
    if (useUnitMeter) {
        delay = 1000.0 * delay / SPEED_OF_SOUND;
    }
    return (uint32_t)std::lround(sampleRate * delay / 1000.0);
}

FilterDelay::FilterDelay() {
    _size = _index = 0;
    _pBuffer = nullptr;
}

FilterDelay::FilterDelay(const uint32_t sampleRate, const double delay, const bool useUnitMeter) {
    init(getSampleDelay(sampleRate, delay, useUnitMeter));
}

FilterDelay::FilterDelay(const uint32_t sampleDelay) {
    init(sampleDelay);
}

FilterDelay::~FilterDelay() {
    delete[] _pBuffer;
}

void FilterDelay::init(const uint32_t sampleDelay) {
    _size = sampleDelay;
    _index = 0;
    _pBuffer = new double[_size];
    reset();
}