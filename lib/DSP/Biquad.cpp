#include "Biquad.h"
#define _USE_MATH_DEFINES
#include <math.h> // M_LN2
#include <cmath> // std::pow

#ifndef LOG_INFO
#include "Log.h"
#endif

Biquad::Biquad() {
    _b0 = _b1 = _b2 = _a0 = _a1 = _a2 = _z1 = _z2 = 0;
}

void Biquad::init(const double b0, const double b1, const double b2, const double a0, const double a1, const double a2) {
    _b0 = b0;
    _b1 = b1;
    _b2 = b2;
    _a0 = a0;
    _a1 = a1;
    _a2 = a2;
	normalize();
}

void Biquad::init(const double b0, const double b1, const double b2, const double a1, const double a2) {
	init(b0, b1, b2, 1.0, a1, a2);
}

void Biquad::initLowPass(const uint32_t sampleRate, const double frequency, const double q) {
	const double w0 = getOmega(sampleRate, frequency);
	const double alpha = getAlpha(w0, q);
	const double cs = std::cos(w0);
    _b0 = (1 - cs) / 2;
    _b1 = 1 - cs;
    _b2 = _b0;
    _a0 = 1 + alpha;
    _a1 = -2 * cs;
    _a2 = 1 - alpha;
	normalize();
}

void Biquad::initLowPass(const uint32_t sampleRate, const double frequency) {
	const double w0 = getOmega(sampleRate, frequency);
	const double sn = std::sin(w0);
	const double cs = std::cos(w0);
    _b0 = sn;
    _b1 = _b0;
    _b2 = 0;
    _a0 = cs + _b0 + 1;
    _a1 = _b0 - cs - 1;
    _a2 = 0;
	normalize();
}

void Biquad::initHighPass(const uint32_t sampleRate, const double frequency, const double q) {
	const double w0 = getOmega(sampleRate, frequency);
	const double alpha = getAlpha(w0, q);
	const double cs = std::cos(w0);
    _b0 = (1 + cs) / 2;
    _b1 = -(1 + cs);
    _b2 = (1 + cs) / 2;
    _a0 = 1 + alpha;
    _a1 = -2 * cs;
    _a2 = 1 - alpha;
	normalize();
}

void Biquad::initHighPass(const uint32_t sampleRate, const double frequency) {
	const double w0 = getOmega(sampleRate, frequency);
	const double sn = std::sin(w0);
	const double cs = std::cos(w0);
    _b0 = cs + 1;
    _b1 = -(cs + 1);
    _b2 = 0;
    _a0 = cs + sn + 1;
    _a1 = sn - cs - 1;
    _a2 = 0;
	normalize();
}

void Biquad::initLowShelf(const uint32_t sampleRate, const double frequency, const double gain, const double q) {
	const double w0 = getOmega(sampleRate, frequency);
	const double A = std::pow(10, gain / 40);
	const double alpha = getAlpha(w0, q);
	const double sqa = std::sqrt(A) * alpha;
	const double cs = std::cos(w0);
    _b0 = A*((A + 1) - (A - 1) * cs + 2 * sqa);
    _b1 = 2 * A*((A - 1) - (A + 1) * cs);
    _b2 = A*((A + 1) - (A - 1) * cs - 2 * sqa);
    _a0 = (A + 1) + (A - 1) * cs + 2 * sqa;
    _a1 = -2 * ((A - 1) + (A + 1) * cs);
    _a2 = (A + 1) + (A - 1) * cs - 2 * sqa;
	normalize();
}

void Biquad::initHighShelf(const uint32_t sampleRate, const double frequency, const double gain, const double q) {
	const double w0 = getOmega(sampleRate, frequency);
	const double A = std::pow(10, gain / 40);
	const double alpha = getAlpha(w0, q);
	const double sqa = std::sqrt(A) * alpha;
	const double cs = std::cos(w0);
    _b0 = A*((A + 1) + (A - 1) * cs + 2 * sqa);
    _b1 = -2 * A*((A - 1) + (A + 1) * cs);
    _b2 = A*((A + 1) + (A - 1) * cs - 2 * sqa);
    _a0 = (A + 1) - (A - 1) * cs + 2 * sqa;
    _a1 = 2 * ((A - 1) - (A + 1) * cs);
    _a2 = (A + 1) - (A - 1) * cs - 2 * sqa;
	normalize();
}

void Biquad::initPEQ(const uint32_t sampleRate, const double frequency, const double q, const double gain) {
	const double w0 = getOmega(sampleRate, frequency);
	const double alpha = getAlpha(w0, q);
	const double A = std::pow(10, gain / 40);
	const double cs = std::cos(w0);
    _b0 = 1 + alpha * A;
    _b1 = -2 * cs;
    _b2 = 1 - alpha * A;
    _a0 = 1 + alpha / A;
    _a1 = -2 * cs;
    _a2 = 1 - alpha / A;
	normalize();
}

void Biquad::initBandPass(const uint32_t sampleRate, const double frequency, const double bandwidth, const double gain) {
	const double w0 = getOmega(sampleRate, frequency);
	const double alpha = std::sin(w0) * std::sinh(M_LN2 / 2 * bandwidth * w0 / std::sin(w0));
	const double A = std::pow(10, gain / 20);
    _b0 = A * alpha;
    _b1 = 0;
    _b2 = -A * alpha;
    _a0 = 1 + alpha;
    _a1 = -2 * std::cos(w0);
    _a2 = 1 - alpha;
	normalize();
}

void Biquad::initNotch(const uint32_t sampleRate, const double frequency, const double bandwidth, const double gain) {
	const double w0 = getOmega(sampleRate, frequency);
	const double alpha = std::sin(w0) * std::sinh(M_LN2 / 2 * bandwidth * w0 / std::sin(w0));
	const double cs = std::cos(w0);
	const double A = std::pow(10, gain / 20);
    _b0 = A;
    _b1 = -2 * cs * A;
    _b2 = A;
    _a0 = 1 + alpha;
    _a1 = -2 * cs;
    _a2 = 1 - alpha;
	normalize();
}

void Biquad::initLinkwitzTransform(const uint32_t sampleRate, const double F0, const double Q0, const double Fp, const double Qp) {
	const double Fc = (F0 + Fp) / 2;
	const double d0i = std::pow(2 * M_PI * F0, 2);
	const double d1i = (2 * M_PI * F0) / Q0;
	const double c0i = std::pow(2 * M_PI * Fp, 2);
	const double c1i = (2 * M_PI * Fp) / Qp;
	const double gn = (2 * M_PI * Fc) / std::tan(M_PI * Fc / sampleRate);
	const double gn2 = std::pow(gn, 2);
	const double cci = c0i + gn * c1i + gn2;
    _b0 = (d0i + gn * d1i + gn2) / cci;
    _b1 = 2 * (d0i - gn2) / cci;
    _b2 = (d0i - gn * d1i + gn2) / cci;
    _a0 = 1;
    _a1 = 2 * (c0i - gn2) / cci;
    _a2 = (c0i - gn * c1i + gn2) / cci;
	reset();
}

double Biquad::getOmega(const uint32_t sampleRate, const double frequency) const {
	return 2 * M_PI * frequency / sampleRate;
}

double Biquad::getAlpha(const double w0, const double q) const {
	return std::sin(w0) / (2 * q);
}

void Biquad::normalize() {
    _b0 /= _a0;
    _b1 /= _a0;
    _b2 /= _a0;
    _a1 /= _a0;
    _a2 /= _a0;
    _a0 = 1;
	reset();
}

const std::vector<std::vector<double>> Biquad::getFrequencyResponse(const uint32_t sampleRate, const uint32_t nPoints, const double fMin, const double fMax) const {
	const double logFreqStep = std::log2(fMax / fMin) / (double)(nPoints - 1);
	std::vector<std::vector<double>> result;
	for (uint32_t i = 0; i < nPoints; ++i) {
		const double f = fMin * std::exp2(i * logFreqStep);
		const double w = 2 * M_PI * f / sampleRate;
		const double phi = 4 * std::pow(std::sin(w / 2), 2);
		const double db = 10 * std::log10(std::pow(_b0 + _b1 + _b2, 2) + (_b0 * _b2 * phi - (_b1 * (_b0 + _b2) + 4 * _b0 * _b2)) * phi) - 10 * std::log10(std::pow(1 + _a1 + _a2, 2) + (1 * _a2 * phi - (_a1 * (1 + _a2) + 4 * 1 * _a2)) * phi);
		result.push_back({ f, db });
	}
	return result;
}

void Biquad::printCoefficients(const bool miniDSPFormat) const {
    LOG_INFO("b0=%.15g,", _b0);
    LOG_INFO("b1=%.15g,", _b1);
    LOG_INFO("b2=%.15g,", _b2);
    LOG_INFO("a1=%.15g,", miniDSPFormat ? -_a1 : _a1);
    LOG_INFO("a2=%.15g,", miniDSPFormat ? -_a2 : _a2);
}