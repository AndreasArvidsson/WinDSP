/*
	This file contains the enum representation of a filter type
	as well as functions to convert trom and to strings

	Author: Andreas Arvidsson
	Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <string>

enum class FilterType {
	LOW_PASS, HIGH_PASS, LOW_SHELF, HIGH_SHELF, PEQ, BAND_PASS, NOTCH, LINKWITZ_TRANSFORM, BIQUAD, FIR, CANCELLATION
};

namespace FilterTypes {

    const FilterType fromString(const std::string &str);
    const std::string toString(const FilterType filterType);

};