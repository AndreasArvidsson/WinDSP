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

namespace CaptureLoop {
	void init(const Config *pConfig, const std::vector<Input*> *pInputs, const std::vector<Output*> *pOutputs, AudioDevice *pCaptureDevice, AudioDevice *pRenderDevice);
	void init(const Config *pConfig, const std::vector<Input*> *pInputs, const std::vector<Output*> *pOutputs, AudioDevice *pCaptureDevice);
	void destroy();
	void run();

	//Private
	extern const Config *_pConfig;
	extern const std::vector<Input*> *_pInputs;
	extern const std::vector<Output*> *_pOutputs;
	extern AudioDevice *_pCaptureDevice;
	extern AudioDevice *_pRenderDevice;
	extern bool *_pUsedChannels;
	extern float _pOverflowBuffer[];
	extern float *_pClippingChannels;
	extern double *_renderBlockBuffer;
	extern UINT32 _overflowSize;
	extern size_t _nChannelsIn, _nChannelsOut;

	void __bufferSwitch(long bufferIndex, ASIOBool processNow);
	void __wasapiLoop();
	void __writeToBuffer(const float* const pSource, const UINT32 length, const int bufferIndex, const UINT32 offset = 0);
}