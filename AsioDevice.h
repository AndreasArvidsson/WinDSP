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
    void startRenderService();
    void stopRenderService();
    void reset();
    std::vector<std::string> getDeviceNames();
    const std::string getName();
    const long getSampleRate();
    const long getNumOutputChannels();
    const long getNumChannels();
    const long getBufferSize();
    double * const getWriteBuffer();
    void addWriteBuffer(double * const pBuffer);
    const bool isRunning();
    void printInfo();

    //Private
    long _asioMessage(const long selector, const long value, void * const message, double * const opt);
    void _bufferSwitch(const long asioBufferIndex, const ASIOBool);
    void _loadDriver(const std::string &driverName);
    void _assertAsio(const ASIOError error);
    void _assertSampleType(const ASIOSampleType type);
    const std::string _asioSampleType(const ASIOSampleType type);
    const std::string _asioResult(const ASIOError error);
    double * const _getReadBuffer();
    void _addReadBuffer(double * const pBuffer);
}