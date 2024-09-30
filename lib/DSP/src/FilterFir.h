#pragma once
#include "Filter.h"
#include <memory>

using std::unique_ptr;

class FilterFir : public Filter {
public:

    FilterFir(const vector<double> &taps);

    const vector<string> toString() const override;

    inline const double process(const double value) override {
        double result = value * _taps[0];
        for (size_t i = _size - 1; i > 0; --i) {
            _pDelay[i] = _pDelay[i - 1];
            result += _pDelay[i] * _taps[i];
        }
        _pDelay[0] = value;
        return result;
    }

    inline void reset() override {
        memset(_pDelay.get(), 0, _size * sizeof(double));
    }

private:
    vector<double> _taps;
    unique_ptr<double[]> _pDelay;
    size_t _size;

};