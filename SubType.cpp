#include "SubType.h"
#include "Error.h"
#include "Str.h"

const SubType SubTypes::fromString(const std::string &strIn) {
    const std::string str = String::toUpperCase(strIn);
    if (str.compare("BUTTERWORTH") == 0) {
        return SubType::BUTTERWORTH;
    }
    else if (str.compare("LINKWITZ_RILEY") == 0) {
        return SubType::LINKWITZ_RILEY;
    }
    else if (str.compare("BESSEL") == 0) {
        return SubType::BESSEL;
    }
    else if (str.compare("CUSTOM") == 0) {
        return SubType::CUSTOM;
    }
    throw Error("Unknown filter sub type '%s'", str.c_str());
}

const std::string SubTypes::toString(const SubType filterType) {
    switch (filterType) {
    case SubType::BUTTERWORTH:
        return "BUTTERWORTH";
    case SubType::LINKWITZ_RILEY:
        return "LINKWITZ_RILEY";
    case SubType::BESSEL:
        return "BESSEL";
    case SubType::CUSTOM:
        return "CUSTOM";
    default:
        throw Error("Unknown filter sub type %d", filterType);
    };
}
