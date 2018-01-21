/*
	This file contains the enum representation of a crossover filter sub type
	as well as functions to convert trom and to strings

	Author: Andreas Arvidsson
	Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <string>
#include "Error.h"

enum class SubType {
    BUTTERWORTH, LINKWITZ_RILEY, BESSEL, CUSTOM
};

class SubTypes {
public:

	static const SubType fromString(const std::string &str) {
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

	static const std::string toString(const SubType filterType) {
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

};