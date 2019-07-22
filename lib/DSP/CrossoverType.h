#pragma once
#include <vector>
#include <string>

enum class CrossoverType {
	Butterworth, Linkwitz_Riley, Bessel
};

class CrossoverTypes {
public:
	static const std::string toString(const CrossoverType type);
	static const CrossoverType fromString(const std::string &value);
	static const std::vector<double> getQValues(const CrossoverType type, const uint8_t order);
    static const std::vector<double> getQValues(const CrossoverType type, const uint8_t order, const double qOffset);

};