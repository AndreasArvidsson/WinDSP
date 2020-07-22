#pragma once
#include <vector>

using std::vector;

namespace Convert {

    const double levelToDb(const double level);
    const double dbToLevel(const double db);

	const vector<double> pcm16ToDouble(const char *pData, const size_t numSamples);
	const vector<double> pcm24ToDouble(const char *pData, const size_t numSamples);
	const vector<double> pcm32ToDouble(const char *pData, const size_t numSamples);
	const vector<double> float32ToDouble(const char *pData, const size_t numSamples);
	const vector<double> float64ToDouble(const char *pData, const size_t numSamples);

};