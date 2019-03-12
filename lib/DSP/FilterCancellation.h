#pragma once
#include "FilterDelay.h"

class FilterCancellation : public Filter {
public:

    FilterCancellation(const uint32_t sampleRate, const double frequency, const double gain = 0.0);
    ~FilterCancellation();

    const std::string toString() const;

    inline const double process(const double data) override {
        return _pFilterDelay->process(data) * _multiplier;
    }

    inline void reset() override {
        _pFilterDelay->reset();
    }

private:
    FilterDelay *_pFilterDelay;
    double _frequency, _gain, _multiplier;

};
