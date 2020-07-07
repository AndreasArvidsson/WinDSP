#pragma once
#include "Filter.h"
#include <cstdint>
//#include <cstring> //memset

class FilterDelay : public Filter {
public:

	static const uint32_t getSampleDelay(const uint32_t sampleRate, double delay, const bool useUnitMeter = false);

	FilterDelay();
	FilterDelay(const uint32_t sampleRate, const double delay, const bool useUnitMeter = false);
	~FilterDelay();

    const std::string toString() const override;

	inline const double process(const double value) override {
		if (_index == _size) {
			_index = 0;
		}
		const double out = _pBuffer[_index];
		_pBuffer[_index++] = value;
		return out;
	}

	inline void reset() override {
		memset(_pBuffer, 0, _size * sizeof(double));
	}

private:
	uint32_t _size, _index;
	double *_pBuffer;
    double _delay;
    bool _useUnitMeter;

};
