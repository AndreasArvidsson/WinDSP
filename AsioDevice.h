/*
    This class represents a single audio ASIO endpoint/device/soundcard.
    All operations having to do with rendering audio to an ASIO device is defined here.

    Author: Andreas Arvidsson
    Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <vector>
#include <mutex>
#include <atomic>
#include "asiosys.h"
#include "asio.h"

namespace AsioDevice {
    //Public
    void destroy();
    std::vector<std::string> getDeviceNames();
    void init(const std::string &driverName, const long bufferSize = 0, const long numChannels = 0);
    void startRenderService();
    void stopRenderService();
    void printInfo();
    const std::string getName();
    const long getSampleRate();
    const long getNumOutputChannels();
    const long getNumChannels();
    const long getBufferSize();
    double* getWriteBuffer();
    void addWriteBuffer(double * const pBuffer);

    void test();

    //Privates
    extern std::string *_pDriverName;
    extern ASIOBufferInfo* _pBufferInfos;
    extern ASIOChannelInfo* _pChannelInfos;
    extern long _numInputChannels, _numOutputChannels, _numChannels, _asioVersion, _driverVersion;
    extern long _minSize, _maxSize, _preferredSize, _granularity, _bufferSize, _inputLatency, _outputLatency;
    extern double _sampleRate;
    extern bool _outputReady;
    extern std::atomic<bool> _run;
    extern std::vector<double*> *_pBuffers, *_pUnusedBuffers;
    extern std::mutex _buffersMutex, _unusedBuffersMutex;
    extern ASIOCallbacks _callbacks;

    extern /*std::atomic<bool> _reset;*/

    long _asioMessage(const long selector, const long value, void * const message, double * const opt);
    void _bufferSwitch(const long asioBufferIndex, const ASIOBool);
    void _loadDriver(const std::string &driverName);
    void _assertAsio(ASIOError error);
    void _assertSampleType(ASIOSampleType type);
    const std::string _asioSampleType(const ASIOSampleType type);
    const std::string _asioResult(const ASIOError error);
    double* _getReadBuffer();
    void _addReadBuffer(double * const pBuffer);

}