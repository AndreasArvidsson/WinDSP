#pragma once
#include <vector>
#include <string>

enum class CrossoverType {
    BUTTERWORTH, LINKWITZ_RILEY, BESSEL, CUSTOM
};

namespace CrossoverTypes {
    const std::string toString(const CrossoverType type);
    const CrossoverType fromString(const std::string& value);
    const std::vector<double> getQValues(const CrossoverType type, const uint8_t order);
    const std::vector<double> getQValues(const CrossoverType type, const uint8_t order, const double qOffset);
};