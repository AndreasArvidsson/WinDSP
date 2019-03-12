#pragma once
#include <string>

class Filter {
public:
	virtual ~Filter() {}

	virtual inline const double process(double value) = 0;
	virtual inline void reset() = 0;
    virtual const std::string toString() const = 0;

};