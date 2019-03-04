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

	Output(const std::string &name);
	~Output();

	void add(OutputFork * const pFork);
	const std::vector<OutputFork*>& getForks() const;
	const std::string getName() const;

	void reset();
	const double resetClipping();

	inline const double process(const double data) {
		double out = 0.0;
		for (const OutputFork *pFork : _forks) {
			out += pFork->process(data);
		}
		if (std::abs(out) > 1.0) {
			//Record clipping level so it can be shown in error message.
			_clipping = std::max(_clipping, std::abs(out));
			//Clamp/limit to max value to avoid damaging equipment.
			return out > 0.0 ? 1.0 : -1.0;
		}
		return out;
	}

private:
	std::string _name;
	std::vector<OutputFork*> _forks;
	double _clipping;

};