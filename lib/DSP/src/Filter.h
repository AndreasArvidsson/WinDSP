#pragma once
#include <string>
#include <vector>

using std::string;
using std::vector;

class Filter {
public:
    virtual ~Filter() {}

    virtual inline const double process(double value) = 0;
    virtual inline void reset() = 0;
    virtual const vector<string> toString() const = 0;

};