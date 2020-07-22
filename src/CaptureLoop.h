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

using std::shared_ptr;
using std::unique_ptr;
using std::thread;
using std::atomic;
using std::vector;

class Config;
class AudioDevice;
class Input;
class Output;

class CaptureLoop {
public:
	CaptureLoop(const shared_ptr<Config> pConfig, unique_ptr<AudioDevice>& pCaptureDevice, unique_ptr<AudioDevice>& pRenderDevice);
	~CaptureLoop();
	void run();

private:
    shared_ptr<Config> _pConfig;
    vector<Input> *_pInputs;
    vector<Output> *_pOutputs;
	unique_ptr<AudioDevice> _pCaptureDevice, _pRenderDevice;
    atomic<bool> _run;
    thread _captureThread;

    void _captureLoopWasapi();
	void _captureLoopAsio();
	void _resetFilters();
	void _checkConfig();
	void _checkClippingChannels();
	void _updateConditionalRouting();
	void _printUsedChannels();
};