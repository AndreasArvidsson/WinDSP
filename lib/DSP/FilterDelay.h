#pragma once
#include "Filter.h"
#include "Constants.h"

class FilterDelay : public Filter {
public:

	static const int getSampleDelay(const uint32_t sampleRate, double delay, const bool useUnitMeter = false) {
		//Value is in meter. Convert to milliseconds
		if (useUnitMeter) {
			delay = 1000.0 * delay / SPEED_OF_SOUND;
		}
		return std::lround(sampleRate * delay / 1000.0);
	}

	FilterDelay() {
		_size = _index = 0;
		_pBuffer = nullptr;
	}

	FilterDelay(const uint32_t sampleRate, const double delay, const bool useUnitMeter = false) {
		init(getSampleDelay(sampleRate, delay, useUnitMeter));
	}

	FilterDelay(const int sampleDelay) {
		init(sampleDelay);
	}

	~FilterDelay() {
		delete[] _pBuffer;
	}

	void init(const int sampleDelay) {
		_size = sampleDelay;
		_index = 0;
		_pBuffer = new double[_size];
		reset();
	}

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

};
