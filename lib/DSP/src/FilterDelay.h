#pragma once
#include "Filter.h"
#include <cstdint>
#include <memory>

using std::unique_ptr;

class FilterDelay : public Filter {
public:

    static const uint32_t getSampleDelay(const uint32_t sampleRate, double delay, const bool useUnitMeter = false);

    FilterDelay();
    FilterDelay(const uint32_t sampleRate, const double delay, const bool useUnitMeter = false);

    const vector<string> toString() const override;

    inline const double process(const double value) override {
        if (_index == _size) {
            _index = 0;
        }
        const double out = _pBuffer[_index];
        _pBuffer[_index++] = value;
        return out;
    }

    inline void reset() override {
        memset(_pBuffer.get(), 0, _size * sizeof(double));
    }

private:
    uint32_t _size, _index;
    unique_ptr<double[]> _pBuffer;
    double _delay;
    bool _useUnitMeter;

};
