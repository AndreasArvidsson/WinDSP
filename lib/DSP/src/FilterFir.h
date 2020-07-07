#pragma once
#include "Filter.h"
#include <vector>

class FilterFir : public Filter {
public:

    FilterFir(const std::vector<double> &taps);
    ~FilterFir();

    const std::string toString() const override;

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
		memset(_pDelay, 0, _size * sizeof(double));
	}

private:
	std::vector<double> _taps;
	double *_pDelay;
	size_t _size;

};