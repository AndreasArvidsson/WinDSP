#pragma once

class SineGenerator {
public:

    SineGenerator(const double sampleRate, const double frequency, const double gain = 0);

    const double next();

private:
    double _phase, _phaseDelta, _amplitude;

};




