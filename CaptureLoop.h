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
	extern bool _silent, _run;
	extern std::thread _wasapiRenderThread, _wasapiCaptureThread;
	extern Buffer _buffers[NUM_BUFFERS];
	extern std::mutex _indexMutex, _silentMutex;
	extern std::mutex _swapMutex;
	extern  std::condition_variable _swapCondition;

	extern size_t _bufferIndexCapture, _bufferIndexRender;


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
		return index + 1 == NUM_BUFFERS ? 0 : index + 1;
	}

	inline const size_t _getBufferIndexCapture() {
		std::lock_guard<std::mutex> lock(_indexMutex);
		return _bufferIndexCapture;
	}

	inline const size_t _getBufferIndexRender() {
		std::lock_guard<std::mutex> lock(_indexMutex);
		return _bufferIndexRender;
	}

	inline void _setBufferIndexCapture(const size_t index) {
		_indexMutex.lock();
		_bufferIndexCapture = index;
		_indexMutex.unlock();
	}

	inline void _setBufferIndexRender(const size_t index) {
		_indexMutex.lock();
		_bufferIndexRender = index;
		_indexMutex.unlock();
	}

	inline void _advanceBufferIndex() {
		_setBufferIndexCapture(_getNextBufferIndex(_getBufferIndexRender()));
		size_t index = _getBufferIndexRender();
		_buffers[index].resetSize();
		_setBufferIndexRender(_getNextBufferIndex(index));
	}

	inline const bool _isSilence() {
		std::lock_guard<std::mutex> lock(_silentMutex);
		return _silent;
	}

	inline void _setSilence(const bool silence) {
		_silentMutex.lock();
		_silent = silence;
		_silentMutex.unlock();
	}

	inline void _resetBuffers() {
		for (int i = 0; i < NUM_BUFFERS; ++i) {
			_buffers[i].fillWithSilence();
		}
		_setBufferIndexCapture(0);
		_setBufferIndexRender(NUM_BUFFERS - 1);
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