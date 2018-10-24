/*
	This class represents a single audio endpoint/device/soundcard.
	All operations having to do with capturing or rendering audio is defined here.
	It's implemented using ASIO.

	Author: Andreas Arvidsson
	Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <vector>
#include <windows.h>

class AsioDrivers;
struct ASIOBufferInfo;
struct ASIOChannelInfo;
struct ASIOCallbacks;

class AsioDevice {
public:
	static void initStatic();
	static void destroyStatic();
	static std::vector<std::string> getDeviceNames();

	AsioDevice(const char* driverName, const long numChannels = -1, const HWND windowHandle = 0);
	~AsioDevice();

	void printInfo() const;
	void startRenderService(ASIOCallbacks *pCallbacks);
	void stopRenderService();
	
	ASIOBufferInfo* getBufferInfos();
	ASIOChannelInfo* getChannelsInfos();
	const long getBufferSize();
	const bool getPostOutput();

private:
	static bool _initStatic;
	static AsioDrivers *_pAsioDrivers;

	ASIOBufferInfo *_pBufferInfos;
	ASIOChannelInfo *_pChannelInfos;
	std::string _name;
	long _asioVersion, _driverVersion;
	long _numInputChannels, _numOutputChannels, _numChannels, _numChannelsRequested;
	long _minSize, _maxSize, _preferredSize, _granularity;
	long _inputLatency, _outputLatency;
	long _bufferSize;
	double _sampleRate;
	bool _postOutput;

};
