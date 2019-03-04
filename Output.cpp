#include "Output.h"

Output::Output(const std::string &name) {
	_name = name;
	_clipping = 0.0;
}

Output::~Output() {
	for (const OutputFork * const pFork : _forks) {
		delete pFork;
	}
}

void Output::add(OutputFork * const pFork) {
	_forks.push_back(pFork);
}

const std::vector<OutputFork*>& Output::getForks() const {
	return _forks;
}

void Output::reset() {
	for (const OutputFork * const pFork : _forks) {
		pFork->reset();
	}
}

const std::string Output::getName() const {
	return _name;
}

const double Output::resetClipping() {
	const double result = _clipping;
	_clipping = 0.0;
	return result;
}