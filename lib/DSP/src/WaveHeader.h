#pragma once
#include <stdint.h>

#define HEADER_NUM_SAMPLES 25000
#define HEADER_NUM_CHANNELS 1
#define HEADER_SAMPLE_RATE 44100
#define HEADER_BIT_DEPTH 16

class WaveHeader {
public:
    char chunkID[4] = { 'R', 'I', 'F', 'F' };
    uint32_t chunkSize;
    char format[4] = { 'W', 'A', 'V', 'E' };
    char subChunk1ID[4] = { 'f', 'm', 't', ' ' };
    uint32_t subChunk1Size = 16;
    uint16_t  audioFormat = 1;
    uint16_t  numChannels = HEADER_NUM_CHANNELS;
    uint32_t sampleRate = HEADER_SAMPLE_RATE;
    uint32_t byteRate = HEADER_NUM_CHANNELS * HEADER_SAMPLE_RATE * HEADER_BIT_DEPTH / 8;
    uint16_t  blockAlign = HEADER_NUM_CHANNELS * HEADER_BIT_DEPTH / 8;
    uint16_t  bitsPerSample = HEADER_BIT_DEPTH;
    char subChunk2ID[4] = { 'd', 'a', 't', 'a' };
    uint32_t subChunk2Size = HEADER_NUM_SAMPLES * HEADER_NUM_CHANNELS * HEADER_BIT_DEPTH / 8;

    WaveHeader() {
        chunkSize = 32 + subChunk2Size;
    }

    const size_t getNumSamples() const  {
        return subChunk2Size / (numChannels * bitsPerSample / 8);
    }

    const size_t getTotalNumSamples() const  {
        return subChunk2Size / (bitsPerSample / 8);
    }

};
