/*
	This class represents the continuous capture loop.
	As long as the application is running this class will:
		1) Capture audio samples from the capture device
		2) Route audio samples to the desired output channels on the render device
		3) Apply filters on routes and/or outputs

	Author: Andreas Arvidsson
	Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <vector>
#include <atomic>
#include "Config.h"
#include "ConfigChangedException.h"
#include "Date.h"
#include "Convert.h"
#include "Keyboard.h"

namespace CaptureLoop {
	
	//Public
	void init(const Config *pConfig, AudioDevice *pCaptureDevice, AudioDevice *pRenderDevice);
	void destroy();
	void run();
	void stop();

	//Private
	extern const Config *_pConfig;
	extern const std::vector<Input*> *_pInputs;
	extern const std::vector<Output*> *_pOutputs;
	extern AudioDevice *_pCaptureDevice;
	extern AudioDevice *_pRenderDevice;
	extern size_t _nChannelsIn, _nChannelsOut, _renderBufferByteSize;
	extern UINT32 _renderBufferCapacity;
	extern std::thread _wasapiRenderThread;
	extern std::atomic<bool> _run, _throwError;
	extern bool _silence;
	extern double *_pProcessBuffer;
	extern bool *_pUsedChannels;
	extern Error _error;

	void _asioRenderCallback(const long bufferIndex, const ASIOBool);
	void _wasapiRenderLoop();
	void _resetFilters();
	void _checkConfig();
	void _checkClippingChannels();
	void _updateConditionalRouting();
	void _printUsedChannels();
	void _fillProcessBuffer();
	long _asioMessage(const long selector, const long value, void* const message, double* const opt);

}