#pragma once
#include "FilterDelay.h"
#include <memory>

using std::unique_ptr;

class FilterCancellation : public Filter {
public:

    FilterCancellation(const uint32_t sampleRate, const double frequency, const double gain = 0.0);

    const vector<string> toString() const override;

    inline const double process(const double data) override {
        return _pFilterDelay->process(data) * _multiplier;
    }

    inline void reset() override {
        _pFilterDelay->reset();
    }

private:
    unique_ptr<FilterDelay> _pFilterDelay;
    double _frequency, _gain, _multiplier;

};
