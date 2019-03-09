#pragma once
#include <stdint.h>
#include <vector>
#define _USE_MATH_DEFINES
#include "math.h"

#define INT24_MAX 8388607i32
#define UINT24_TO_INT24 16777216i32

class Convert {
public:

	static inline const double levelToDb(const double level) {
		return 20 * log10(level);
	}

	static const std::vector<double> pcm16ToDouble(const char *pData, const size_t numSamples) {
		std::vector<double> result(numSamples);
        const int16_t *p = (int16_t*)pData;
		for (size_t i = 0; i < numSamples; ++i) {
			result[i] = (double)p[i] / INT16_MAX;
		}
		return result;
	}

	static const std::vector<double> pcm24ToDouble(const char *pData, const size_t numSamples) {
		std::vector<double> result(numSamples);
		const unsigned char *p = (const unsigned char*)pData;
		double int32_t;
		for (size_t i = 0; i < numSamples; ++i) {
			int32_t = p[2] << 16 | p[1] << 8 | p[0];
			if (int32_t > INT24_MAX) {
				result[i] = (double)(int32_t - UINT24_TO_INT24) / INT24_MAX;
			}
			else {
				result[i] = (double)int32_t / INT24_MAX;
			}
			p += 3;
		}
		return result;
	}

	static const std::vector<double> pcm32ToDouble(const char *pData, const size_t numSamples) {
		std::vector<double> result(numSamples);
        const int32_t *p = (int32_t*)pData;
		for (size_t i = 0; i < numSamples; ++i) {
			result[i] = (double)p[i] / INT32_MAX;
		}
		return result;
	}

	static const std::vector<double> float32ToDouble(const char *pData, const size_t numSamples) {
		std::vector<double> result(numSamples);
		const float *p = (float*)pData;
		for (size_t i = 0; i < numSamples; ++i) {
			result[i] = (double)p[i];
		}
		return result;
	}

	static const std::vector<double> float64ToDouble(const char *pData, const size_t numSamples) {
		std::vector<double> result(numSamples);
        const double *p = (double*)pData;
		for (size_t i = 0; i < numSamples; ++i) {
			result[i] = p[i];
		}
		return result;
	}

};