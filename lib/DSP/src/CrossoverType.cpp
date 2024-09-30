#include "CrossoverType.h"
#include <cmath> // sqrt
#include "Error.h"
#include "Str.h"

using std::sqrt;

const CrossoverType CrossoverTypes::fromString(const string& strIn) {
    const string str = String::toUpperCase(strIn);
    if (str.compare("BUTTERWORTH") == 0) {
        return CrossoverType::BUTTERWORTH;
    }
    else if (str.compare("LINKWITZ_RILEY") == 0) {
        return CrossoverType::LINKWITZ_RILEY;
    }
    else if (str.compare("BESSEL") == 0) {
        return CrossoverType::BESSEL;
    }
    else if (str.compare("CUSTOM") == 0) {
        return CrossoverType::CUSTOM;
    }
    throw Error("Unknown filter sub type '%s'", strIn.c_str());
}

const string CrossoverTypes::toString(const CrossoverType crossoverType) {
    switch (crossoverType) {
    case CrossoverType::BUTTERWORTH:
        return "BUTTERWORTH";
    case CrossoverType::LINKWITZ_RILEY:
        return "LINKWITZ_RILEY";
    case CrossoverType::BESSEL:
        return "BESSEL";
    case CrossoverType::CUSTOM:
        return "CUSTOM";
    default:
        throw Error("Unknown filter sub type %d", crossoverType);
    };
}

const vector<double> CrossoverTypes::getQValues(const CrossoverType type, const uint8_t order) {
    switch (type) {
    case CrossoverType::BUTTERWORTH:
        switch (order) {
        case 1:
            return vector<double> { -1 };
        case 2:
            return vector<double> { sqrt(0.5) };
        case 3:
            return vector<double> { -1, 1 };
        case 4:
            return vector<double> { 1 / 1.8478, 1 / 0.7654  };
        case 5:
            return vector<double> { -1, 1 / 0.6180, 1 / 1.6180 };
        case 6:
            return vector<double> { 1 / 1.9319, sqrt(0.5), 1 / 0.5176  };
        case 7:
            return vector<double> { -1, 1 / 1.8019, 1 / 1.2470, 1 / 0.4450 };
        case 8:
            return vector<double> { 1 / 1.96161, 1 / 1.6629, 1 / 1.1111, 1 / 0.3902 };
        }
        break;

    case CrossoverType::LINKWITZ_RILEY:
        switch (order) {
        case 2:
            return vector<double> { -1, -1  };
        case 4:
            return vector<double> { sqrt(0.5), sqrt(0.5) };
        case 8:
            return vector<double> { 1 / 1.8478, 1 / 0.7654, 1 / 1.8478, 1 / 0.7654 };
        }
        break;

    case CrossoverType::BESSEL:
        switch (order) {
        case 2:
            return vector<double> { 0.57735026919 };
        case 3:
            return vector<double> { -1, 0.691046625825 };
        case 4:
            return vector<double> { 0.805538281842, 0.521934581669 };
        case 5:
            return vector<double> { -1, 0.916477373948, 0.563535620851 };
        case 6:
            return vector<double> { 1.02331395383, 0.611194546878, 0.510317824749 };
        case 7:
            return vector<double> { -1, 1.12625754198, 0.660821389297, 0.5323556979 };
        case 8:
            return vector<double> { 1.22566942541, 0.710852074442, 0.559609164796, 0.505991069397 };
        }
        break;

    default:
        throw Error("Unknown crossover type: %s", toString(type).c_str());
    }
    throw Error("Crossover type '%s' have no order '%d'", toString(type).c_str(), order);
}

const vector<double> CrossoverTypes::getQValues(const CrossoverType type, const uint8_t order, const double qOffset) {
    vector<double> values = getQValues(type, order);
    const double qMultiplier = 1 + qOffset;
    for (size_t i = 0; i < values.size(); ++i) {
        if (values[i] >= 0) {
            values[i] *= qMultiplier;
        }
    }
    return values;
}