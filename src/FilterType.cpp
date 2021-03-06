#include "FilterType.h"
#include "Error.h"
#include "Str.h"

const FilterType FilterTypes::fromString(const string& strIn) {
    const string str = String::toUpperCase(strIn);
    if (str.compare("LOW_PASS") == 0) {
        return FilterType::LOW_PASS;
    }
    else if (str.compare("HIGH_PASS") == 0) {
        return FilterType::HIGH_PASS;
    }
    else if (str.compare("LOW_SHELF") == 0) {
        return FilterType::LOW_SHELF;
    }
    else if (str.compare("HIGH_SHELF") == 0) {
        return FilterType::HIGH_SHELF;
    }
    else if (str.compare("PEQ") == 0) {
        return FilterType::PEQ;
    }
    else if (str.compare("BAND_PASS") == 0) {
        return FilterType::BAND_PASS;
    }
    else if (str.compare("NOTCH") == 0) {
        return FilterType::NOTCH;
    }
    else if (str.compare("LINKWITZ_TRANSFORM") == 0) {
        return FilterType::LINKWITZ_TRANSFORM;
    }
    else if (str.compare("BIQUAD") == 0) {
        return FilterType::BIQUAD;
    }
    else if (str.compare("FIR") == 0) {
        return FilterType::FIR;
    }
    else if (str.compare("CANCELLATION") == 0) {
        return FilterType::CANCELLATION;
    }
    else if (str.compare("COMPRESSION") == 0) {
        return FilterType::COMPRESSION;
    }
    throw Error("Unknown filter type '%s'", str.c_str());
}

const string FilterTypes::toString(const FilterType filterType) {
    switch (filterType) {
    case FilterType::LOW_PASS:
        return "LOW_PASS";
    case FilterType::HIGH_PASS:
        return "HIGH_PASS";
    case FilterType::LOW_SHELF:
        return "LOW_SHELF";
    case FilterType::HIGH_SHELF:
        return "HIGH_SHELF";
    case FilterType::PEQ:
        return "PEQ";
    case FilterType::BAND_PASS:
        return "BAND_PASS";
    case FilterType::NOTCH:
        return "NOTCH";
    case FilterType::LINKWITZ_TRANSFORM:
        return "LINKWITZ_TRANSFORM";
    case FilterType::BIQUAD:
        return "BIQUAD";
    case FilterType::FIR:
        return "FIR";
    case FilterType::CANCELLATION:
        return "CANCELLATION";
    case FilterType::COMPRESSION:
        return "COMPRESSION";
    default:
        throw Error("Unknown filter type %d", filterType);
    }
}