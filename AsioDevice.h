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
	extern ASIOBufferInfo* pBufferInfos;
	extern ASIOChannelInfo* pChannelInfos;
	extern long numInputChannels, numOutputChannels, numChannels, asioVersion, driverVersion;
	extern long minSize, maxSize, preferredSize, granularity, bufferSize, inputLatency, outputLatency;
	extern double sampleRate;
	extern bool outputReady;

	void destroy();
	std::vector<std::string> getDeviceNames();
	void init(const std::string &driverName, const HWND windowHandle = nullptr);
	void startRenderService(ASIOCallbacks *pCallbacks, const long bufferSize = 0, const long numChannels = 0);
	void stopRenderService();
	void renderSilence(const long bufferIndex);
	void printInfo();
	const std::string getName();

	//Privates
	extern std::string *_pDriverName;

	long _asioMessage(const long selector, const long value, void* const message, double* const opt);
	void _loadDriver(const std::string &driverName, const HWND windowHandle);
}