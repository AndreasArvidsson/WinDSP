/*
	This file contains the enum representation of a crossover filter sub type
	as well as functions to convert trom and to strings

	Author: Andreas Arvidsson
	Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <string>

enum class SubType {
    BUTTERWORTH, LINKWITZ_RILEY, BESSEL, CUSTOM
};

namespace SubTypes {

    const SubType fromString(const std::string &str);
    const std::string toString(const SubType filterType);

};