#pragma once
#include <stdint.h>
#include <vector>
#define _USE_MATH_DEFINES
#include "math.h"

class Convert {
public:

	static const std::vector<double> pcm16ToDouble(const char *pData, const size_t numSamples) {
		std::vector<double> result(numSamples);
		int16_t *p = (int16_t*)pData;
		for (size_t i = 0; i < numSamples; ++i) {
			result[i] = (double)p[i] / INT16_MAX;
		}
		return result;
	}

	static const std::vector<double> pcm32ToDouble(const char *pData, const size_t numSamples) {
		std::vector<double> result(numSamples);
		int32_t *p = (int32_t*)pData;
		for (size_t i = 0; i < numSamples; ++i) {
			result[i] = (double)p[i] / INT32_MAX;
		}
		return result;
	}

	static const std::vector<double> float32ToDouble(const char *pData, const size_t numSamples) {
		std::vector<double> result(numSamples);
		float *p = (float*)pData;
		for (size_t i = 0; i < numSamples; ++i) {
			result[i] = (double)p[i];
		}
		return result;
	}

	static const std::vector<double> float64ToDouble(const char *pData, const size_t numSamples) {
		std::vector<double> result(numSamples);
		double *p = (double*)pData;
		for (size_t i = 0; i < numSamples; ++i) {
			result[i] = p[i];
		}
		return result;
	}

};