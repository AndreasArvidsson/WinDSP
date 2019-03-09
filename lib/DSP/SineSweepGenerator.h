#pragma once

class SineSweepGenerator {
public:

    SineSweepGenerator(const size_t sampleRate, const double frequencyStart, const double frequencyEnd, const double duration, const double gain = 0);

    const double getFrequency() const;
    const size_t getNumSamples() const;

    const double next();
    void reset();

private:
    double _phase, _frequencyStart, _frequency, _phaseDelta, _freqDelta, _amplitude;
    size_t _index, _numSamples, _sampleRate;

    const void updatePhaseDelta();

};