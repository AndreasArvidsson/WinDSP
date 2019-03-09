#include "FilterDelay.h"
#include "Constants.h"
#include <cmath> //lround

const int FilterDelay::getSampleDelay(const uint32_t sampleRate, double delay, const bool useUnitMeter) {
    //Value is in meter. Convert to milliseconds
    if (useUnitMeter) {
        delay = 1000.0 * delay / SPEED_OF_SOUND;
    }
    return std::lround(sampleRate * delay / 1000.0);
}

FilterDelay::FilterDelay() {
    _size = _index = 0;
    _pBuffer = nullptr;
}

FilterDelay::FilterDelay(const uint32_t sampleRate, const double delay, const bool useUnitMeter) {
    init(getSampleDelay(sampleRate, delay, useUnitMeter));
}

FilterDelay::FilterDelay(const int sampleDelay) {
    init(sampleDelay);
}

FilterDelay::~FilterDelay() {
    delete[] _pBuffer;
}

void FilterDelay::init(const int sampleDelay) {
    _size = sampleDelay;
    _index = 0;
    _pBuffer = new double[_size];
    reset();
}