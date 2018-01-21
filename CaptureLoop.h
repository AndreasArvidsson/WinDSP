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

class CaptureLoop {
public:

	CaptureLoop(Config *pConfig, const AudioDevice *pCaptureDevice, const AudioDevice *pRenderDevice, const std::vector<Input*> &inputs, const std::vector<Output*> &outputs);
	~CaptureLoop();

	void capture();

private:
	Config *_pConfig;
	const AudioDevice *_pCaptureDevice, *_pRenderDevice;
	std::vector<Input*> _inputs;
	std::vector<Output*> _outputs;
	size_t _nChannelsIn, _nChannelsOut;
	time_t *_pUsedChannels;
	float *_pClippingChannels;

	inline void checkConfig() {
		if (_pConfig->hasChanged()) {
			throw ConfigChangedException();
		}
	}

	inline void checkUsedChannels() {
		const time_t now = Date::getCurrentTimeMillis();
		//Compare now against last used timestamp and determine active channels
		for (size_t i = 0; i < _nChannelsIn; ++i) {
			if (now - _pUsedChannels[i] > 1000) {
				_pUsedChannels[i] = 0;
			}
		}
		//Update conditional routing
		for (const Input *pInut : _inputs) {
			pInut->evalConditions();
		}
	}

	inline void checkClippingChannels() {
		for (size_t i = 0; i < _nChannelsOut; ++i) {
			if (_pClippingChannels[i] != 0) {
				printf("WARNING: Output(%s) - Clipping detected: +%0.2f dBFS\n", _pConfig->getChannelName(i).c_str(), Convert::levelToDb(_pClippingChannels[i]));
				_pClippingChannels[i] = 0;
			}
		}
	}

};