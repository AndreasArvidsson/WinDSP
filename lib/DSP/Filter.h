#pragma once

class Filter {
public:
	virtual ~Filter() {}

	virtual const double process(double value) = 0;

};