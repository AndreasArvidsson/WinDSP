#include "FilterBiquad.h"
#include "Biquad.h"
#include "CrossoverType.h"
#include "Str.h"

#ifndef LOG_INFO
#include "Log.h"
#endif

FilterBiquad::FilterBiquad(const uint32_t sampleRate) {
    _sampleRate = sampleRate;
}

const size_t FilterBiquad::size() const {
    return _biquads.size();
}

const bool FilterBiquad::isEmpty() const {
    return _biquads.size() == 0;
}

const uint32_t FilterBiquad::getSampleRate() const {
    return _sampleRate;
}

void FilterBiquad::add(const double b0, const double b1, const double b2, const double a1, const double a2) {
    Biquad biquad;
    biquad.init(b0, b1, b2, a1, a2);
    _biquads.push_back(biquad);
    _toStringValue.push_back(String::format(
        "Biquad5: b0 %s, b1 %s, b2 %s, a1 %s, a2 %s",
        String::toString(b0).c_str(),
        String::toString(b1).c_str(),
        String::toString(b2).c_str(),
        String::toString(a1).c_str(),
        String::toString(a2).c_str()
    ));
}

void FilterBiquad::add(const double b0, const double b1, const double b2, const double a0, const double a1, const double a2) {
    Biquad biquad;
    biquad.init(b0, b1, b2, a0, a1, a2);
    _biquads.push_back(biquad);
    _toStringValue.push_back(String::format(
        "Biquad6: b0 %s, b1 %s, b2 %s, a0 %s, a1 %s, a2 %s",
        String::toString(b0).c_str(),
        String::toString(b1).c_str(),
        String::toString(b2).c_str(),
        String::toString(a0).c_str(),
        String::toString(a1).c_str(),
        String::toString(a2).c_str()
    ));
}

void FilterBiquad::addCrossover(const bool isLowPass, const double frequency, const vector<double>& qValues) {
    if (isLowPass) {
        addLowPass(frequency, qValues);
    }
    else {
        addHighPass(frequency, qValues);
    }
}

void FilterBiquad::addCrossover(const bool isLowPass, const double frequency, const CrossoverType type, const uint8_t order, const double qOffset) {
    if (isLowPass) {
        addLowPass(frequency, type, order, qOffset);
    }
    else {
        addHighPass(frequency, type, order, qOffset);
    }
}

void FilterBiquad::addLowPass(const double frequency, const vector<double>& qValues) {
    int order = 0;
    for (double q : qValues) {
        Biquad biquad;
        // First order biquad
        if (q < 0) {
            biquad.initLowPass(_sampleRate, frequency);
            order += 1;
        }
        // Second order biquad
        else {
            biquad.initLowPass(_sampleRate, frequency, q);
            order += 2;
        }
        _biquads.push_back(biquad);
    }
    _toStringValue.push_back(String::format(
        "Lowpass: %sHz, %ddB/oct, Q %s",
        String::toString(frequency).c_str(),
        order * 6,
        String::join(qValues).c_str()
    ));
}

void FilterBiquad::addLowPass(const double frequency, const CrossoverType type, const uint8_t order, const double qOffset) {
    addLowPass(frequency, CrossoverTypes::getQValues(type, order, qOffset));
}

void FilterBiquad::addHighPass(const double frequency, const vector<double>& qValues) {
    int order = 0;
    for (double q : qValues) {
        Biquad biquad;
        if (q < 0) {
            biquad.initHighPass(_sampleRate, frequency);
            order += 1;
        }
        else {
            biquad.initHighPass(_sampleRate, frequency, q);
            order += 2;
        }
        _biquads.push_back(biquad);
    }
    _toStringValue.push_back(String::format(
        "Highpass: %sHz, %ddB/oct, Q %s",
        String::toString(frequency).c_str(),
        order * 6,
        String::join(qValues).c_str()
    ));
}

void FilterBiquad::addHighPass(const double frequency, const CrossoverType type, const uint8_t order, const double qOffset) {
    addHighPass(frequency, CrossoverTypes::getQValues(type, order, qOffset));
}

void FilterBiquad::addShelf(const bool isLowShelf, const double frequency, const double gain, const double q) {
    if (isLowShelf) {
        addLowShelf(frequency, gain, q);
    }
    else {
        addHighShelf(frequency, gain, q);
    }
}

void FilterBiquad::addLowShelf(const double frequency, const double gain, const double q) {
    Biquad biquad;
    biquad.initLowShelf(_sampleRate, frequency, gain, q);
    _biquads.push_back(biquad);
    _toStringValue.push_back(String::format(
        "Lowshelf: freq %sHz, gain %sdB, Q %s",
        String::toString(frequency).c_str(),
        String::toString(gain).c_str(),
        String::toString(q).c_str()
    ));
}

void FilterBiquad::addHighShelf(const double frequency, const double gain, const double q) {
    Biquad biquad;
    biquad.initHighShelf(_sampleRate, frequency, gain, q);
    _biquads.push_back(biquad);
    _toStringValue.push_back(String::format(
        "Highshelf: freq %sHz, gain %sdB, Q %s",
        String::toString(frequency).c_str(),
        String::toString(gain).c_str(),
        String::toString(q).c_str()
    ));
}

void FilterBiquad::addPEQ(const double frequency, const double gain, const double q) {
    Biquad biquad;
    biquad.initPEQ(_sampleRate, frequency, gain, q);
    _biquads.push_back(biquad);
    _toStringValue.push_back(String::format(
        "PEQ: freq %sHz, gain %sdB, Q %s",
        String::toString(frequency).c_str(),
        String::toString(gain).c_str(),
        String::toString(q).c_str()
    ));
}

void FilterBiquad::addBandPass(const double frequency, const double bandwidth, const double gain) {
    Biquad biquad;
    biquad.initBandPass(_sampleRate, frequency, bandwidth, gain);
    _biquads.push_back(biquad);
    _toStringValue.push_back(String::format(
        "Bandpass: freq %sHz, gain %sdB, bandwidth %s",
        String::toString(frequency).c_str(),
        String::toString(gain).c_str(),
        String::toString(bandwidth).c_str()
    ));
}

void FilterBiquad::addNotch(const double frequency, const double bandwidth, const double gain) {
    Biquad biquad;
    biquad.initNotch(_sampleRate, frequency, bandwidth, gain);
    _biquads.push_back(biquad);
    _toStringValue.push_back(String::format(
        "Notch: freq %sHz, gain %sdB, bandwidth %s",
        String::toString(frequency).c_str(),
        String::toString(gain).c_str(),
        String::toString(bandwidth).c_str()
    ));
}

void FilterBiquad::addLinkwitzTransform(const double f0, const double q0, const double fp, const double qp) {
    Biquad biquad;
    biquad.initLinkwitzTransform(_sampleRate, f0, q0, fp, qp);
    _biquads.push_back(biquad);
    _toStringValue.push_back(String::format(
        "Linkwitz Transform: f0 %sHz, Q0 %s, fp %sHz, qp %s",
        String::toString(f0).c_str(),
        String::toString(q0).c_str(),
        String::toString(fp).c_str(),
        String::toString(qp).c_str()
    ));
}

const vector<vector<double>> FilterBiquad::getFrequencyResponse(const uint32_t nPoints, const double fMin, const double fMax) const {
    vector<vector<double>> result(nPoints);
    for (const Biquad& biquad : _biquads) {
        const vector<vector<double>> data = biquad.getFrequencyResponse(_sampleRate, nPoints, fMin, fMax);
        for (uint32_t i = 0; i < nPoints; ++i) {
            // First use of this index/Hz. Use entire Hz/dB pair.
            if (result[i].size() == 0) {
                result[i] = data[i];
            }
            // Index/Hz already exists. Just add level/dB.
            else {
                result[i][1] += data[i][1];
            }
        }
    }
    return result;
}

void FilterBiquad::printCoefficients(const bool miniDSPFormat) const {
    int index = 1;
    for (const Biquad& biquad : _biquads) {
        LOG_INFO("biquad%d,", index++);
        biquad.printCoefficients(miniDSPFormat);
    }
    LOG_NL();
}

const vector<string> FilterBiquad::toString() const {
    return _toStringValue;
}