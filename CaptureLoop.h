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

class AsioDevice;

namespace Capture {
	void init(const Config *pConfig, const std::vector<Input*> *pInputs, const std::vector<Output*> *pOutputs, AudioDevice *pCaptureDevice, AudioDevice *pRenderDevice);
	void init(const Config *pConfig, const std::vector<Input*> *pInputs, const std::vector<Output*> *pOutputs, AudioDevice *pCaptureDevice, AsioDevice *pRenderDevice);
	void destroy();
	void run();
}