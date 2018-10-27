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
#include "Config.h"
#include "ConfigChangedException.h"
#include "Date.h"
#include "Convert.h"
#include "Keyboard.h"

#include <vector>

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
	extern double *_pOverflowBuffer;
	extern double *_pClippingChannels;
	extern double *_renderTmpBuffer;
	extern bool *_pUsedChannels;
	extern UINT32 _overflowSize;
	extern size_t _nChannelsIn, _nChannelsOut, _renderBlockSize;
	extern bool _clippingDetected, _silent, _run;
	extern std::thread _wasapiRenderThread;

	void _bufferSwitch(const long bufferIndex, const ASIOBool);
	void _wasapiLoop();
	void _writeToBuffer(const double* const pSource, const UINT32 length, const int bufferIndex, const UINT32 offset = 0);
	void _fillRenderTmpBuffer(float* captureBuffer, const UINT32 length);
	void _getCaptureBuffer(UINT32 * const pNumFramesAvailable, bool * const pSilenceToPlayback = nullptr);
	void _resetFilters();
	void _checkConfig();
	void _checkClippingChannels();
	void _updateConditionalRouting();
	void _printUsedChannels();

}