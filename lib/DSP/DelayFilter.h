#pragma once
#include "Filter.h"

class DelayFilter : public Filter {
public:

	static const int getSampleDelay(const uint32_t sampleRate, double delay, const bool useUnitMeter = false) {
		//Value is in meter. Convert to milliseconds
		if (useUnitMeter) {
			delay = 1000 * delay / 343.0;
		}
		return std::lround(sampleRate * delay / 1000);
	}

	DelayFilter() {
		_size = _index = 0;
		_pBuffer = nullptr;
	}

	DelayFilter(const uint32_t sampleRate, const double delay, const bool useUnitMeter = false) {
		init(getSampleDelay(sampleRate, delay, useUnitMeter));
	}

	DelayFilter(const int sampleDelay) {
		init(sampleDelay);
	}

	~DelayFilter() {
		delete[] _pBuffer;
	}

	void init(const int sampleDelay) {
		_size = sampleDelay;
		_index = 0;
		_pBuffer = new double[_size];
		reset();
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

};
