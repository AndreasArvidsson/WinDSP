/*
    This class represents a single audio ASIO endpoint/device/soundcard.
    All operations having to do with rendering audio to an ASIO device is defined here.

    Author: Andreas Arvidsson
    Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <vector>
#include <string>
#include <memory>
#include "asiosys.h"
#include "asio.h"

using std::unique_ptr;
using std::string;
using std::vector;

namespace AsioDevice {
    //Public
    void initRenderService(const string &driverName, const long sampleRate, const long bufferSize = 0, const long numChannels = 0, const bool inDebug = false);
    void destroy();
    void startService();
    void stopService();
    void reset();
    vector<string> getDeviceNames();
    const string getName();
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
    unique_ptr<double[]> _getWriteBuffer();
    unique_ptr<double[]> _getReadBuffer();
    void _releaseWriteBuffer(unique_ptr<double[]>& pBuffer);
    void _releaseReadBuffer(unique_ptr<double[]>& pBuffer);
    void _renderSilence(const long asioBufferIndex);
    void _loadDriver(const string &driverName);
    void _assertAsio(const ASIOError error);
    void _assertSampleType(const ASIOSampleType type);
    const string _asioSampleType(const ASIOSampleType type);
    const string _asioResult(const ASIOError error);
}