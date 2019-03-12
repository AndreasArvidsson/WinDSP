#include "Biquad.h"
#define _USE_MATH_DEFINES
#include <math.h> // M_LN2

Biquad::Biquad() {
	b0 = b1 = b2 = a0 = a1 = a2 = z1 = z2 = 0;
}

void Biquad::init(const double b0, const double b1, const double b2, const double a0, const double a1, const double a2) {
	this->b0 = b0;
	this->b1 = b1;
	this->b2 = b2;
	this->a0 = a0;
	this->a1 = a1;
	this->a2 = a2;
	normalize();
}

void Biquad::init(const double b0, const double b1, const double b2, const double a1, const double a2) {
	init(b0, b1, b2, 1.0, a1, a2);
}

void Biquad::initLowPass(const uint32_t sampleRate, const double frequency, const double q) {
	const double w0 = getOmega(sampleRate, frequency);
	const double alpha = getAlpha(w0, q);
	const double cs = std::cos(w0);
	b0 = (1 - cs) / 2;
	b1 = 1 - cs;
	b2 = b0;
	a0 = 1 + alpha;
	a1 = -2 * cs;
	a2 = 1 - alpha;
	normalize();
}

void Biquad::initLowPass(const uint32_t sampleRate, const double frequency) {
	const double w0 = getOmega(sampleRate, frequency);
	const double sn = std::sin(w0);
	const double cs = std::cos(w0);
	b0 = sn;
	b1 = b0;
	b2 = 0;
	a0 = cs + b0 + 1;
	a1 = b0 - cs - 1;
	a2 = 0;
	normalize();
}

void Biquad::initHighPass(const uint32_t sampleRate, const double frequency, const double q) {
	const double w0 = getOmega(sampleRate, frequency);
	const double alpha = getAlpha(w0, q);
	const double cs = std::cos(w0);
	b0 = (1 + cs) / 2;
	b1 = -(1 + cs);
	b2 = (1 + cs) / 2;
	a0 = 1 + alpha;
	a1 = -2 * cs;
	a2 = 1 - alpha;
	normalize();
}

void Biquad::initHighPass(const uint32_t sampleRate, const double frequency) {
	const double w0 = getOmega(sampleRate, frequency);
	const double sn = std::sin(w0);
	const double cs = std::cos(w0);
	b0 = cs + 1;
	b1 = -(cs + 1);
	b2 = 0;
	a0 = cs + sn + 1;
	a1 = sn - cs - 1;
	a2 = 0;
	normalize();
}

void Biquad::initLowShelf(const uint32_t sampleRate, const double frequency, const double gain, const double q) {
	const double w0 = getOmega(sampleRate, frequency);
	const double A = std::pow(10, gain / 40);
	const double alpha = getAlpha(w0, q);
	const double sqa = std::sqrt(A) * alpha;
	const double cs = std::cos(w0);
	b0 = A*((A + 1) - (A - 1) * cs + 2 * sqa);
	b1 = 2 * A*((A - 1) - (A + 1) * cs);
	b2 = A*((A + 1) - (A - 1) * cs - 2 * sqa);
	a0 = (A + 1) + (A - 1) * cs + 2 * sqa;
	a1 = -2 * ((A - 1) + (A + 1) * cs);
	a2 = (A + 1) + (A - 1) * cs - 2 * sqa;
	normalize();
}

void Biquad::initHighShelf(const uint32_t sampleRate, const double frequency, const double gain, const double q) {
	const double w0 = getOmega(sampleRate, frequency);
	const double A = std::pow(10, gain / 40);
	const double alpha = getAlpha(w0, q);
	const double sqa = std::sqrt(A) * alpha;
	const double cs = std::cos(w0);
	b0 = A*((A + 1) + (A - 1) * cs + 2 * sqa);
	b1 = -2 * A*((A - 1) + (A + 1) * cs);
	b2 = A*((A + 1) + (A - 1) * cs - 2 * sqa);
	a0 = (A + 1) - (A - 1) * cs + 2 * sqa;
	a1 = 2 * ((A - 1) - (A + 1) * cs);
	a2 = (A + 1) - (A - 1) * cs - 2 * sqa;
	normalize();
}

void Biquad::initPEQ(const uint32_t sampleRate, const double frequency, const double q, const double gain) {
	const double w0 = getOmega(sampleRate, frequency);
	const double alpha = getAlpha(w0, q);
	const double A = std::pow(10, gain / 40);
	const double cs = std::cos(w0);
	b0 = 1 + alpha * A;
	b1 = -2 * cs;
	b2 = 1 - alpha * A;
	a0 = 1 + alpha / A;
	a1 = -2 * cs;
	a2 = 1 - alpha / A;
	normalize();
}

void Biquad::initBandPass(const uint32_t sampleRate, const double frequency, const double bandwidth, const double gain) {
	const double w0 = getOmega(sampleRate, frequency);
	const double alpha = std::sin(w0) * std::sinh(M_LN2 / 2 * bandwidth * w0 / std::sin(w0));
	const double A = std::pow(10, gain / 20);
	b0 = A * alpha;
	b1 = 0;
	b2 = -A * alpha;
	a0 = 1 + alpha;
	a1 = -2 * std::cos(w0);
	a2 = 1 - alpha;
	normalize();
}

void Biquad::initNotch(const uint32_t sampleRate, const double frequency, const double bandwidth, const double gain) {
	const double w0 = getOmega(sampleRate, frequency);
	const double alpha = std::sin(w0) * std::sinh(M_LN2 / 2 * bandwidth * w0 / std::sin(w0));
	const double cs = std::cos(w0);
	const double A = std::pow(10, gain / 20);
	b0 = A;
	b1 = -2 * cs * A;
	b2 = A;
	a0 = 1 + alpha;
	a1 = -2 * cs;
	a2 = 1 - alpha;
	normalize();
}

void Biquad::initLinkwitzTransform(const uint32_t sampleRate, const double F0, const double Q0, const double Fp, const double Qp) {
	const double Fc = (F0 + Fp) / 2;
	const double d0i = std::pow(2 * M_PI * F0, 2);
	const double d1i = (2 * M_PI * F0) / Q0;
	const double c0i = std::pow(2 * M_PI * Fp, 2);
	const double c1i = (2 * M_PI * Fp) / Qp;
	const double gn = (2 * M_PI * Fc) / std::tan(M_PI * Fc / sampleRate);
	const double pw = std::pow(gn, 2);
	const double cci = c0i + gn * c1i + std::pow(gn, 2);
	b0 = (d0i + gn * d1i + pw) / cci;
	b1 = 2 * (d0i - pw) / cci;
	b2 = (d0i - gn * d1i + pw) / cci;
	a0 = 1;
	a1 = 2 * (c0i - pw) / cci;
	a2 = (c0i - gn * c1i + pw) / cci;
	reset();
}

double Biquad::getOmega(const uint32_t sampleRate, const double frequency) const {
	return 2 * M_PI * frequency / sampleRate;
}

double Biquad::getAlpha(const double w0, const double q) const {
	return std::sin(w0) / (2 * q);
}

void Biquad::normalize() {
	b0 /= a0;
	b1 /= a0;
	b2 /= a0;
	a1 /= a0;
	a2 /= a0;
	a0 = 1;
	reset();
}

const std::vector<std::vector<double>> Biquad::getFrequencyResponse(const uint32_t sampleRate, const uint32_t nPoints, const double fMin, const double fMax) const {
	const double logFreqStep = std::log2(fMax / fMin) / (double)(nPoints - 1);
	std::vector<std::vector<double>> result;
	for (uint32_t i = 0; i < nPoints; ++i) {
		const double f = fMin * std::exp2(i * logFreqStep);
		const double w = 2 * M_PI * f / sampleRate;
		const double phi = 4 * std::pow(std::sin(w / 2), 2);
		const double db = 10 * std::log10(std::pow(b0 + b1 + b2, 2) + (b0 * b2 * phi - (b1 * (b0 + b2) + 4 * b0 * b2)) * phi) - 10 * std::log10(std::pow(1 + a1 + a2, 2) + (1 * a2 * phi - (a1 * (1 + a2) + 4 * 1 * a2)) * phi);
		result.push_back({ f, db });
	}
	return result;
}

void Biquad::printCoefficients(const bool miniDSPFormat) const {
    printf("b0=%.15g,\n", b0);
    printf("b1=%.15g,\n", b1);
    printf("b2=%.15g,\n", b2);
    printf("a1=%.15g,\n", miniDSPFormat ? -a1 : a1);
    printf("a2=%.15g,\n", miniDSPFormat ? -a2 : a2);
}