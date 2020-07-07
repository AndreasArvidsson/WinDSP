/*
    This class represents a single audio ASIO endpoint/device/soundcard.
    All operations having to do with rendering audio to an ASIO device is defined here.

    Author: Andreas Arvidsson
    Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <vector>
#include <string>
#include "asiosys.h"
#include "asio.h"

class Config;

namespace AsioDevice {
    //Public
    void initRenderService(const Config *pConfig, const std::string &driverName, const long sampleRate, const long bufferSize = 0, const long numChannels = 0);
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
    double * const _getReadBuffer();
    void _releaseWriteBuffer(double * const pBuffer);
    void _releaseReadBuffer(double * const pBuffer);
    void _renderSilence(const long asioBufferIndex);
    void _loadDriver(const std::string &driverName);
    void _assertAsio(const ASIOError error);
    void _assertSampleType(const ASIOSampleType type);
    const std::string _asioSampleType(const ASIOSampleType type);
    const std::string _asioResult(const ASIOError error);
}