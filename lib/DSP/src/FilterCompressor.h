#pragma once
#include <cmath>
#include <string>
#include "Filter.h"
#include "Convert.h"

/*
    Downward dynamic range compression
*/

class FilterCompressor : public Filter {
public:

    FilterCompressor(
        const uint32_t sampleRate, // samples/sec
        const double threshold, // dB
        const double ratio, // [0.0, 1.0] 0.0=oo:1, 0.5=2:1, 1.0=1:1
        const double attack, // ms
        const double release, // ms
        const double window // ms
        );

    const std::string toString() const;

    inline const double process(const double sample) override {
        const double sum = sample * sample;
        run(sum, _windowCoef, _squaredSum);
        const double rms = sqrt(_squaredSum);
        double over = rms / _threshold;
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
        const double gain = over ? std::pow(over, _ratio - 1) : 1;
        return sample * gain;
    }

    inline void reset() override { }

private:
    double  _threshold, _ratio, _attackCoef, _releaseCoef, _windowCoef, _envelope, _squaredSum;

    inline void run(const double in, const double coef, double& state) const {
        state = in + coef * (state - in);
    }

};


