/*
    Downward dynamic range compression
*/

#pragma once
#include <cmath>
#include <string>
#include "Filter.h"
#include "Convert.h"

using std::pow;
using std::sqrt;

class FilterCompression : public Filter {
public:

    FilterCompression(
        const uint32_t sampleRate, // samples/sec
        const double threshold, // dB
        const double ratio, // [0.0, 1.0] 0.0=oo:1, 0.5=2:1, 1.0=1:1
        const double attack, // ms
        const double release, // ms
        const double window = 0 // ms
    );

    const vector<string> toString() const;

    inline const double process(const double sample) override {
        double over;
        //Use a sliding rms window to calculate how much the sample is over the threshold.
        if (_useWindow) {
            run(sample * sample, _windowCoef, _squaredSum);
            over = sqrt(_squaredSum) / _threshold;
        }
        //Use just the single sample.
        else {
            over = sample * sample / _threshold;
        }
        if (over < 1) {
            over = 1;
        }
        //Attack
        if (over > _envelope) {
            run(over, _attackCoef, _envelope);
        }
        //Release
        else {
            run(over, _releaseCoef, _envelope);
        }
        return sample * pow(over, _ratio);
    }

    inline void reset() override { }

private:
    double  _threshold, _ratio, _attackCoef, _releaseCoef, _windowCoef, _envelope, _squaredSum;
    bool _useWindow;
    string _toStringValue;

    inline void run(const double in, const double coef, double& state) const {
        state = in + coef * (state - in);
    }

    const string getToStringValue(
        const double threshold,
        const double ratio,
        const double attack,
        const double release,
        const double window);

};


