#include "CrossoverType.h"
#include "Error.h"
#include <cmath> // std::sqrt

const std::string CrossoverTypes::toString(const CrossoverType type) {
	switch (type) {
	case CrossoverType::Butterworth:
		return "Butterworth";
	case CrossoverType::Linkwitz_Riley:
		return "Linkwitz-Riley";
	case CrossoverType::Bessel:
		return "Bessel";
	default:
		throw Error("Unknown crossover type: %d", type);
	}
}

const CrossoverType CrossoverTypes::fromString(const std::string &value) {
	if (value.compare("Butterworth") == 0) {
		return CrossoverType::Butterworth;
	}
	else if (value.compare("Linkwitz-Riley") == 0) {
		return CrossoverType::Linkwitz_Riley;
	}
	else if (value.compare("Bessel") == 0) {
		return CrossoverType::Bessel;
	}
	throw Error("Unknown crossover type: %s", value.c_str());
}

const std::vector<double> CrossoverTypes::getQValues(const CrossoverType type, const uint8_t order) {
	switch (type) {
	case CrossoverType::Butterworth:
		switch (order) {
		case 1:
			return std::vector<double> { -1 };
		case 2:
			return std::vector<double> { std::sqrt(0.5) };
		case 3:
			return std::vector<double> { -1, 1 };
		case 4:
			return std::vector<double> { 1 / 1.8478, 1 / 0.7654  };
		case 5:
			return std::vector<double> { -1, 1 / 0.6180, 1 / 1.6180 };
		case 6:
			return std::vector<double> { 1 / 1.9319, std::sqrt(0.5), 1 / 0.5176  };
		case 7:
			return std::vector<double> { -1, 1 / 1.8019, 1 / 1.2470, 1 / 0.4450 };
		case 8:
			return std::vector<double> { 1 / 1.96161, 1 / 1.6629, 1 / 1.1111, 1 / 0.3902 };
		}
		break;

	case CrossoverType::Linkwitz_Riley:
		switch (order) {
		case 2:
			return std::vector<double> { -1, -1  };
		case 4:
			return std::vector<double> { std::sqrt(0.5), std::sqrt(0.5) };
		case 8:
			return std::vector<double> { 1 / 1.8478, 1 / 0.7654, 1 / 1.8478, 1 / 0.7654 };
		}
		break;

	case CrossoverType::Bessel:
		switch (order) {
		case 2:
			return std::vector<double> { 0.57735026919 };
		case 3:
			return std::vector<double> { -1, 0.691046625825 };
		case 4:
			return std::vector<double> { 0.805538281842, 0.521934581669 };
		case 5:
			return std::vector<double> { -1, 0.916477373948, 0.563535620851 };
		case 6:
			return std::vector<double> { 1.02331395383, 0.611194546878, 0.510317824749 };
		case 7:
			return std::vector<double> { -1, 1.12625754198, 0.660821389297, 0.5323556979 };
		case 8:
			return std::vector<double> { 1.22566942541, 0.710852074442, 0.559609164796, 0.505991069397 };
		}
		break;

	default:
		throw Error("Unknown crossover type: %s", toString(type).c_str());
	}
	throw Error("Crossover type '%s' have no order '%d'", toString(type).c_str(), order);
}

const std::vector<double> CrossoverTypes::getQValues(const CrossoverType type, const uint8_t order, const double qOffset) {
    std::vector<double> values = getQValues(type, order);
    const double qMultiplier = 1 + qOffset;
    for (size_t i = 0; i < values.size(); ++i) {
        if (values[i] >= 0) {
            values[i] *= qMultiplier;
        }
    }
    return values;
}