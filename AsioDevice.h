/*
    This class represents a single audio ASIO endpoint/device/soundcard.
    All operations having to do with rendering audio to an ASIO device is defined here.

    Author: Andreas Arvidsson
    Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <vector>
#include "asiosys.h"
#include "asio.h"

namespace AsioDevice {
    //Public
    void initRenderService(const std::string &driverName, const long sampleRate, const long bufferSize = 0, const long numChannels = 0);
    void destroy();
    void startService();
    void stopService();
    void reset();
    std::vector<std::string> getDeviceNames();
    const std::string getName();
    const long getSampleRate();
    const long getNumOutputChannels();
    const long getNumChannels();
    const long getBufferSize();
    const bool isRunning();
    void throwError();
    void addSample(const double sample);
    void printInfo();

    //Private
    void _bufferSwitch(const long asioBufferIndex, const ASIOBool);
    long _asioMessage(const long selector, const long value, void * const message, double * const opt);
    double * const _getWriteBuffer();
    void _addWriteBuffer(double * const pBuffer);
    double * const _getReadBuffer();
    void _addReadBuffer(double * const pBuffer);
    void _loadDriver(const std::string &driverName);
    void _assertAsio(const ASIOError error);
    void _assertSampleType(const ASIOSampleType type);
    const std::string _asioSampleType(const ASIOSampleType type);
    const std::string _asioResult(const ASIOError error);
}