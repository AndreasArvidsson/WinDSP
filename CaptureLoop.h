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
#include "Config.h"
#include "ConfigChangedException.h"
#include "Date.h"
#include "Convert.h"
#include "Keyboard.h"
#include "Buffer.h"
#include "Semaphore.h"

#define NUM_BUFFERS 3

namespace CaptureLoop {
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
	extern size_t _nChannelsIn, _nChannelsOut, _renderBlockSize;
	extern UINT32 _renderBufferCapacity;
	extern std::thread _wasapiRenderThread, _wasapiCaptureThread;
	extern Buffer _buffers[NUM_BUFFERS];
	extern std::condition_variable _swapCondition;
	extern size_t _bufferIndexRender;
	extern std::atomic<bool> _run;
	extern Semaphore _swap;

	void _bufferSwitch(const long bufferIndex, const ASIOBool);
	void _wasapiCaptureLoop();
	void _wasapiRenderLoop();
	const size_t _getNextBufferIndex(const size_t index);
	void _resetFilters();
	void _checkConfig();
	void _checkClippingChannels();
	void _updateConditionalRouting();
	void _printUsedChannels();

	inline void processCaptureBuffer(const float *pCaptureBuffer, size_t captureAvailable, size_t bufferIndex);

}