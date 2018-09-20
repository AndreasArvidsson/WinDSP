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

class CaptureLoop {
public:

	CaptureLoop(Config *pConfig, const AudioDevice *pCaptureDevice, const AudioDevice *pRenderDevice, const std::vector<Input*> &inputs, const std::vector<Output*> &outputs);
	~CaptureLoop();

	void capture();

private:
	Config * _pConfig;
	const AudioDevice *_pCaptureDevice, *_pRenderDevice;
	std::vector<Input*> _inputs;
	std::vector<Output*> _outputs;
	size_t _nChannelsIn, _nChannelsOut;
	time_t *_pUsedChannels;
	float *_pClippingChannels;

	void checkConfig();
	void updateConditionalRouting(const time_t now);
	void checkClippingChannels();
	void resetFilters();

};