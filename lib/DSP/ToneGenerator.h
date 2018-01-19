#pragma once
#include "stdint.h"
#define _USE_MATH_DEFINES
#include "math.h"

class ToneGenerator {
public:

	template <typename T>
	static void sine(uint8_t *pBuffer, size_t bufferLength, size_t numChannels, size_t sampleRate, double frequency) {
		double sampleIncrement = (frequency * (M_PI * 2)) / (double)sampleRate;
		T *pDataBuffer = reinterpret_cast<T*>(pBuffer);
		double sinValue;
		//double theta = 0;
		for (size_t i = 0; i < bufferLength / sizeof(T); i += numChannels) {
			sinValue = sin(theta);
			for (size_t j = 0; j < numChannels; j++) {
				pDataBuffer[i + j] = convert<T>(sinValue);
			}
			theta += sampleIncrement;
		}
	}

private:

	static double theta;

	template<typename T>
	static T convert(double Value) {
		return (T)(Value);
	};

	template<>
	static float convert<float>(double Value) {
		return (float)(Value);
	};

	template<>
	static short convert<short>(double Value) {
		return (short)(Value * INT16_MAX);
	};

};

