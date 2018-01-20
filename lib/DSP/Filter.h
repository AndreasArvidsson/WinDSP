#pragma once

class Filter {
public:
	virtual ~Filter() {}

	virtual inline const double process(double value) = 0;

};