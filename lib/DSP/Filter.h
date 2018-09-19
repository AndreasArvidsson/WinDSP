#pragma once

class Filter {
public:
	virtual ~Filter() {}

	virtual inline const double process(double value) = 0;
	virtual inline void reset() = 0;

};