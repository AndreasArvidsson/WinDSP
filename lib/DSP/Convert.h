#pragma once
#include <vector>

namespace Convert {

    const double levelToDb(const double level);
    const double dbToLevel(const double db);

	const std::vector<double> pcm16ToDouble(const char *pData, const size_t numSamples);
	const std::vector<double> pcm24ToDouble(const char *pData, const size_t numSamples);
	const std::vector<double> pcm32ToDouble(const char *pData, const size_t numSamples);
	const std::vector<double> float32ToDouble(const char *pData, const size_t numSamples);
	const std::vector<double> float64ToDouble(const char *pData, const size_t numSamples);

};