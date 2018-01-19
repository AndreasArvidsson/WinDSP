#pragma once
#include <stdint.h>

#define NUM_SAMPLES 25000
#define NUM_CHANNELS 1
#define SAMPLE_RATE 44100
#define BIT_DEPTH 16

class WaveHeader {
public:
	char chunkID[4] = { 'R', 'I', 'F', 'F' };
	uint32_t chunkSize;
	char format[4] = { 'W', 'A', 'V', 'E' };
	char subChunk1ID[4] = { 'f', 'm', 't', ' ' };
	uint32_t subChunk1Size = 16;
	uint16_t  audioFormat = 1;
	uint16_t  numChannels = NUM_CHANNELS;
	uint32_t sampleRate = SAMPLE_RATE;
	uint32_t byteRate = NUM_CHANNELS * SAMPLE_RATE * BIT_DEPTH / 8;
	uint16_t  blockAlign = NUM_CHANNELS * BIT_DEPTH / 8;
	uint16_t  bitsPerSample = BIT_DEPTH;
	char subChunk2ID[4] = { 'd', 'a', 't', 'a' };
	uint32_t subChunk2Size = NUM_SAMPLES * NUM_CHANNELS * BIT_DEPTH / 8;

	WaveHeader() {
		chunkSize = 32 + subChunk2Size;
	}

	size_t getNumSamples() {
		return subChunk2Size / (numChannels * bitsPerSample / 8);
	}

	size_t getTotalNumSamples() {
		return subChunk2Size / (bitsPerSample / 8);
	}

};
