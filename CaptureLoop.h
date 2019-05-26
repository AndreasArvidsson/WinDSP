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
#include <thread>

class Config;
class AudioDevice;
class Input;
class Output;

namespace CaptureLoop {
	//Public
	void init(const Config *pConfig, AudioDevice *pCaptureDevice, AudioDevice *pRenderDevice);
	void destroy();
	void run();
	void stop();

	//Private
    void _captureLoopWasapi();
	void _captureLoopAsio();
	void _resetFilters();
	void _checkConfig();
	void _checkClippingChannels();
	void _updateConditionalRouting();
	void _printUsedChannels();
}