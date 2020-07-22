#pragma once
#include <vector>
#include <string>

using std::string;
using std::vector;

enum class CrossoverType {
    BUTTERWORTH, LINKWITZ_RILEY, BESSEL, CUSTOM
};

namespace CrossoverTypes {
    const string toString(const CrossoverType type);
    const CrossoverType fromString(const string& value);
    const vector<double> getQValues(const CrossoverType type, const uint8_t order);
    const vector<double> getQValues(const CrossoverType type, const uint8_t order, const double qOffset);
};