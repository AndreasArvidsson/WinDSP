#include "Convert.h"

#define INT24_MAX 8388607i32
#define UINT24_TO_INT24 16777216i32

const double Convert::levelToDb(const double level) {
    return 20 * log10(level);
}

const double Convert::dbToLevel(const double db) {
    return std::pow(10, db / 20);
}

const std::vector<double> Convert::pcm16ToDouble(const char *pData, const size_t numSamples) {
    std::vector<double> result(numSamples);
    const int16_t *p = (int16_t*)pData;
    for (size_t i = 0; i < numSamples; ++i) {
        result[i] = (double)p[i] / INT16_MAX;
    }
    return result;
}

const std::vector<double> Convert::pcm24ToDouble(const char *pData, const size_t numSamples) {
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

const std::vector<double> Convert::pcm32ToDouble(const char *pData, const size_t numSamples) {
    std::vector<double> result(numSamples);
    const int32_t *p = (int32_t*)pData;
    for (size_t i = 0; i < numSamples; ++i) {
        result[i] = (double)p[i] / INT32_MAX;
    }
    return result;
}

const std::vector<double> Convert::float32ToDouble(const char *pData, const size_t numSamples) {
    std::vector<double> result(numSamples);
    const float *p = (float*)pData;
    for (size_t i = 0; i < numSamples; ++i) {
        result[i] = (double)p[i];
    }
    return result;
}

const std::vector<double> Convert::float64ToDouble(const char *pData, const size_t numSamples) {
    std::vector<double> result(numSamples);
    const double *p = (double*)pData;
    for (size_t i = 0; i < numSamples; ++i) {
        result[i] = p[i];
    }
    return result;
}