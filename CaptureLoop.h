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
#include <mutex>
#include "Config.h"
#include "ConfigChangedException.h"
#include "Date.h"
#include "Convert.h"
#include "Keyboard.h"
#include "Buffer.h"
#include <atomic>

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
	extern size_t _nChannelsIn, _nChannelsOut, _renderBlockSize, _renderBufferCapacity;
	extern std::thread _wasapiRenderThread, _wasapiCaptureThread;
	extern Buffer _buffers[NUM_BUFFERS];
	extern std::mutex _swapMutex;
	extern  std::condition_variable _swapCondition;
	extern std::atomic<size_t> _bufferIndexCapture, _bufferIndexRender;
	extern std::atomic<bool> _silent;
	extern bool _run;

	void _bufferSwitch(const long bufferIndex, const ASIOBool);
	void _wasapiCaptureLoop();
	void _wasapiRenderLoop();
	void _writeToBuffer(const float* const pSource, const size_t bufferIndex, const size_t length, const size_t offset = 0);

	void _resetFilters();
	void _checkConfig();
	void _checkClippingChannels();
	void _updateConditionalRouting();
	void _printUsedChannels();

	inline const size_t _getNextBufferIndex(const size_t index) {
		return index + 1 < NUM_BUFFERS ? index + 1 : 0;
	}

	inline void _advanceBufferIndex() {
		_bufferIndexCapture = _getNextBufferIndex(_bufferIndexCapture);
		_buffers[_bufferIndexRender].resetSize();
		_bufferIndexRender = _getNextBufferIndex(_bufferIndexRender);
	}

	inline void _resetBuffers() {
		for (Buffer &buf : _buffers) {
			buf.fillWithSilence();
			buf.resetSize();
		}

		//_buffers[_bufferIndexRender].fillWithSilence();
		//_buffers[_bufferIndexCapture].resetSize();

	/*	_buffers[0].fillWithSilence();
		_setBufferIndexRender(0);
		_setBufferIndexCapture(1);*/
	}

	inline void _notifySwap() {
		std::lock_guard<decltype(_swapMutex)> lock(_swapMutex);
		_swapCondition.notify_one();
	}

	inline void _waitSwap() {
		std::unique_lock<decltype(_swapMutex)> lock(_swapMutex);
		_swapCondition.wait(lock);
	}

}