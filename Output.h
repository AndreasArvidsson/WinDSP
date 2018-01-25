/*
	This class represents a single output on the render device.
	Contains all forks specified in the configuration.

	Author: Andreas Arvidsson
	Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <vector>
#include "OutputFork.h"

class Output {
public:
	std::vector<OutputFork*> forks;
	bool mute;

	Output() {
		mute = false;
		forks.push_back(new OutputFork());
	}

	Output(const bool mute) {
		this->mute = mute;
	}

	~Output() {
		for (OutputFork *pFork : forks) {
			delete pFork;
		}
	}

	inline const double process(const double data) const {
		if (mute) {
			return 0;
		}
		double out = 0;
		for (const OutputFork *pFork : forks) {
			out += pFork->process(data);
		}
		return out;
	}

};