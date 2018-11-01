/*
	This class represents a single audio ASIO endpoint/device/soundcard.
	All operations having to do with rendering audio to an ASIO device is defined here.

	Author: Andreas Arvidsson
	Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <string>
#include <vector>
#include <Windows.h> //HWND

class AsioDrivers;
struct ASIOBufferInfo;
struct ASIOChannelInfo;
struct ASIOCallbacks;

namespace AsioDevice {
	//Public
	void destroy();
	std::vector<std::string> getDeviceNames();
	void init(const std::string &driverName, const HWND windowHandle = nullptr);
	void startRenderService(ASIOCallbacks *pCallbacks, const long bufferSize = 0, const long numChannels = 0);
	void stopRenderService();
	void renderSilence(const long bufferIndex);
	void outputReady();
	void printInfo();
	const std::string getName();
	const long getSampleRate();
	const long getNumOutputChannels();
	int* getBuffer(const size_t channelIndex, const long bufferIndex);

	//Privates
	extern std::string *_pDriverName;
	extern ASIOBufferInfo* _pBufferInfos;
	extern ASIOChannelInfo* _pChannelInfos;
	extern long _numInputChannels, _numOutputChannels, _numChannels, _asioVersion, _driverVersion;
	extern long _minSize, _maxSize, _preferredSize, _granularity, _bufferSize, _inputLatency, _outputLatency;
	extern double _sampleRate;
	extern bool _outputReady;

	long _asioMessage(const long selector, const long value, void* const message, double* const opt);
	void _loadDriver(const std::string &driverName, const HWND windowHandle);
}