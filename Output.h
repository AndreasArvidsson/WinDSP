/*
	This class represents a single output on the render device.
	Contains all forks specified in the configuration.

	Author: Andreas Arvidsson
	Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <vector>
#include <algorithm> //std::max
#include "OutputFork.h"

class Output {
public:
	double clipping;

	Output(const std::string &name) {
		_name = name;
		clipping = 0.0;
	}

	~Output() {
		for (OutputFork *pFork : _forks) {
			delete pFork;
		}
	}

	void add(OutputFork* pFork) {
		_forks.push_back(pFork);
	}

	const std::vector<OutputFork*>& getForks() const {
		return _forks;
	}

	inline const double process(const double data) {
		double out = 0;
		for (const OutputFork *pFork : _forks) {
			out += pFork->process(data);
		}
		if (std::abs(out) > 1.0) {
			clipping = std::max(clipping, std::abs(out));
		}
		return out;
	}

	inline void reset() {
		for (const OutputFork *pFork : _forks) {
			pFork->reset();
		}
	}

	const std::string getName() const {
		return _name;
	}

private:
	std::string _name;
	std::vector<OutputFork*> _forks;

};