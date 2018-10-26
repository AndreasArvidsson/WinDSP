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
	extern AsioDrivers* pAsioDrivers;
	extern ASIOBufferInfo* pBufferInfos;
	extern ASIOChannelInfo* pChannelInfos;
	extern long numInputChannels, numOutputChannels, numChannels, asioVersion, driverVersion;
	extern long minSize, maxSize, preferredSize, granularity, bufferSize, inputLatency, outputLatency;
	extern double sampleRate;
	extern bool outputReady;
	extern std::string driverName;

	void destroyStatic();
	std::vector<std::string> getDeviceNames();
	void init(char* const driverName, const long nChannels = -1, const HWND windowHandle = nullptr);
	void startRenderService(ASIOCallbacks *pCallbacks);
	void stopRenderService();
	void printInfo();

	//Privates
	void __initStatic();
	long __asioMessage(const long selector, const long value, void* const message, double* const opt);
	void __loadDriver(char* const dName, const HWND windowHandle);
	void __loadNumChannels(const long nChannels);
}