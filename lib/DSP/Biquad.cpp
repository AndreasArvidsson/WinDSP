#include "Biquad.h"
#include <cmath>

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
	reset();
}

void Biquad::init(const double b0, const double b1, const double b2, const double a1, const double a2) {
	this->b0 = b0;
	this->b1 = b1;
	this->b2 = b2;
	this->a0 = 1.0;
	this->a1 = a1;
	this->a2 = a2;
	reset();
}

void Biquad::initLowPass(const uint32_t sampleRate, const double frequency, const double q) {
	double w0 = getOmega(sampleRate, frequency);
	double alpha = getAlpha(w0, q);
	double cs = std::cos(w0);
	b0 = (1 - cs) / 2;
	b1 = 1 - cs;
	b2 = b0;
	a0 = 1 + alpha;
	a1 = -2 * cs;
	a2 = 1 - alpha;
	normalize();
}

void Biquad::initLowPass(const uint32_t sampleRate, const double frequency) {
	double w0 = getOmega(sampleRate, frequency);
	double sn = sin(w0);
	double cs = std::cos(w0);
	b0 = sn;
	b1 = b0;
	b2 = 0;
	a0 = cs + b0 + 1;
	a1 = b0 - cs - 1;
	a2 = 0;
	normalize();
}

void Biquad::initHighPass(const uint32_t sampleRate, const double frequency, const double q) {
	double w0 = getOmega(sampleRate, frequency);
	double alpha = getAlpha(w0, q);
	double cs = std::cos(w0);
	b0 = (1 + cs) / 2;
	b1 = -(1 + cs);
	b2 = (1 + cs) / 2;
	a0 = 1 + alpha;
	a1 = -2 * cs;
	a2 = 1 - alpha;
	normalize();
}

void Biquad::initHighPass(const uint32_t sampleRate, const double frequency) {
	double w0 = getOmega(sampleRate, frequency);
	double sn = sin(w0);
	double cs = std::cos(w0);
	b0 = cs + 1;
	b1 = -(cs + 1);
	b2 = 0;
	a0 = cs + sn + 1;
	a1 = sn - cs - 1;
	a2 = 0;
	normalize();
}

void Biquad::initLowShelf(const uint32_t sampleRate, const double frequency, const double gain, const double slope) {
	double w0 = getOmega(sampleRate, frequency);
	double A = pow(10, gain / 40);
	double alpha = sin(w0) / 2 * sqrt((A + 1 / A)*(1 / slope - 1) + 2);
	b0 = A*((A + 1) - (A - 1)*cos(w0) + 2 * sqrt(A)*alpha);
	b1 = 2 * A*((A - 1) - (A + 1)*cos(w0));
	b2 = A*((A + 1) - (A - 1)*cos(w0) - 2 * sqrt(A)*alpha);
	a0 = (A + 1) + (A - 1)*cos(w0) + 2 * sqrt(A)*alpha;
	a1 = -2 * ((A - 1) + (A + 1)*cos(w0));
	a2 = (A + 1) + (A - 1)*cos(w0) - 2 * sqrt(A)*alpha;
	normalize();
}

void Biquad::initHighShelf(const uint32_t sampleRate, const double frequency, const double gain, const double slope) {
	double w0 = getOmega(sampleRate, frequency);
	double A = pow(10, gain / 40);
	double alpha = sin(w0) / 2 * sqrt((A + 1 / A)*(1 / slope - 1) + 2);
	b0 = A*((A + 1) + (A - 1)*cos(w0) + 2 * sqrt(A)*alpha);
	b1 = -2 * A*((A - 1) + (A + 1)*cos(w0));
	b2 = A*((A + 1) + (A - 1)*cos(w0) - 2 * sqrt(A)*alpha);
	a0 = (A + 1) - (A - 1)*cos(w0) + 2 * sqrt(A)*alpha;
	a1 = 2 * ((A - 1) - (A + 1)*cos(w0));
	a2 = (A + 1) - (A - 1)*cos(w0) - 2 * sqrt(A)*alpha;
	normalize();
}

void Biquad::initPEQ(const uint32_t sampleRate, const double frequency, const double q, const double gain) {
	double w0 = getOmega(sampleRate, frequency);
	double alpha = getAlpha(w0, q);
	double A = std::sqrt(std::pow(10, gain / 20));
	double sn = sin(w0);
	double cs = std::cos(w0);
	b0 = 1 + alpha * A;
	b1 = -2 * cs;
	b2 = 1 - alpha * A;
	a0 = 1 + alpha / A;
	a1 = -2 * cs;
	a2 = 1 - alpha / A;
	normalize();
}

void Biquad::initBandPass(const uint32_t sampleRate, const double frequency, const double bandwidth, const double gain) {
	double w0 = getOmega(sampleRate, frequency);
	double alpha = sin(w0) * sinh(M_LN2 / 2 * bandwidth * w0 / sin(w0));
	b0 = gain * alpha;
	b1 = 0;
	b2 = -gain * alpha;
	a0 = 1 + alpha;
	a1 = -2 * cos(w0);
	a2 = 1 - alpha;
	normalize();
}

void Biquad::initNotch(const uint32_t sampleRate, const double frequency, const double bandwidth) {
	double w0 = getOmega(sampleRate, frequency);
	double alpha = sin(w0) * sinh(M_LN2 / 2 * bandwidth * w0 / sin(w0));
	double cs = cos(w0);
	b0 = 1;
	b1 = -2 * cs;
	b2 = 1;
	a0 = 1 + alpha;
	a1 = -2 * cs;
	a2 = 1 - alpha;
	normalize();
}

void Biquad::initLinkwitzTransform(const uint32_t sampleRate, const double F0, const double Q0, const double Fp, const double Qp) {
	double Fc = (F0 + Fp) / 2;
	double d0i = std::pow(2 * M_PI * F0, 2);
	double d1i = (2 * M_PI * F0) / Q0;
	double c0i = std::pow(2 * M_PI * Fp, 2);
	double c1i = (2 * M_PI * Fp) / Qp;
	double gn = (2 * M_PI * Fc) / std::tan(M_PI * Fc / sampleRate);
	double cci = c0i + gn * c1i + std::pow(gn, 2);
	b0 = (d0i + gn * d1i + std::pow(gn, 2)) / cci;
	b1 = 2 * (d0i - std::pow(gn, 2)) / cci;
	b2 = (d0i - gn * d1i + std::pow(gn, 2)) / cci;
	a0 = 1;
	a1 = 2 * (c0i - std::pow(gn, 2)) / cci;
	a2 = (c0i - gn * c1i + std::pow(gn, 2)) / cci;
	reset();
}

double Biquad::getOmega(const uint32_t sampleRate, const double frequency) const {
	return  2 * M_PI * frequency / sampleRate;
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

