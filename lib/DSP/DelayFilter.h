#pragma once
#include "Filter.h"

class DelayFilter : public Filter {
public:

	static const int getSampleDelay(const uint32_t sampleRate, double value, const bool useUnitMeter = false) {
		//Value is in meter. Convert to milliseconds
		if (useUnitMeter) {
			value = 1000 * value / 343;
		}
		return std::lround(sampleRate * value / 1000);
	}

	DelayFilter(const uint32_t sampleRate, const double value, const bool useUnitMeter = false) {
		init(getSampleDelay(sampleRate, value, useUnitMeter));
	}

	DelayFilter(const int sampleDelay) {
		init(sampleDelay);
	}

	~DelayFilter() {
		delete[] _pBuffer;
	}

	inline const double process(double value) override {
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

	void init(const int sampleDelay) {
		_size = sampleDelay;
		_index = 0;
		_pBuffer = new double[_size];
		reset();
	}

};
