#pragma once
#include "Filter.h"

class FilterGain : public Filter {
public:

    static const double getMultiplier(const double gain);

    FilterGain(const double gain);
    FilterGain(const double gain, const bool invert);

    const double getGain() const;
    const double getMultiplier() const;
    const double getMultiplierNoInvert() const;
    const bool getInvert() const;
    const vector<string> toString() const override;
    void setGain(const double gain);

    inline const double process(const double value) override {
        return _multiplier * value;
    }

    inline void reset() override { }

private:
    double _gain, _multiplier, _multiplierNoInvert;
    bool _invert;

    void init(const double gain, const bool invert);

};
