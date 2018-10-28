#include "CaptureLoop.h"
#include "AsioDevice.h"
#include "OS.h"


//TODO
#include "SineGenerator.h"
#include "Stopwatch.h"
Stopwatch sw;


#define MAX_INT32 2147483648.0

const Config *CaptureLoop::_pConfig;
const std::vector<Input*> *CaptureLoop::_pInputs = nullptr;
const std::vector<Output*> *CaptureLoop::_pOutputs = nullptr;
AudioDevice *CaptureLoop::_pCaptureDevice = nullptr;
AudioDevice *CaptureLoop::_pRenderDevice = nullptr;
size_t CaptureLoop::_nChannelsIn = 0;
size_t CaptureLoop::_nChannelsOut = 0;
size_t CaptureLoop::_renderBlockSize = 0;
bool CaptureLoop::_silent = false;
bool CaptureLoop::_run = false;
std::thread CaptureLoop::_wasapiCaptureThread;
std::thread CaptureLoop::_wasapiRenderThread;
Buffer CaptureLoop::_buffers[NUM_BUFFERS];

size_t CaptureLoop::_renderBufferCapacity = 0;
std::mutex CaptureLoop::_indexMutex;
std::mutex CaptureLoop::_silentMutex;

size_t CaptureLoop::_bufferIndexCapture = 0;
size_t CaptureLoop::_bufferIndexRender = 0;
std::mutex CaptureLoop::_swapMutex;
std::condition_variable CaptureLoop::_swapCondition;

void CaptureLoop::init(const Config *pConfig, AudioDevice *pCaptureDevice, AudioDevice *pRenderDevice) {
	_pConfig = pConfig;
	_pInputs = pConfig->getInputs();
	_pOutputs = pConfig->getOutputs();
	_pCaptureDevice = pCaptureDevice;
	_pRenderDevice = pRenderDevice;
	_nChannelsIn = _pInputs->size();
	_nChannelsOut = _pOutputs->size();
	_run = _silent = false;

	//_clippingDetected = _silent = _run = false;

	//The size of all sample frames for all channels with the same sample index/timestamp
	_renderBlockSize = sizeof(double) * _nChannelsOut;

	//Used to temporarily store sample(for all out channels) while they are being processed.
	//_renderBlockBuffer = new double[_nChannelsOut];

	//Initialize clipping detection
	//_pClippingChannels = new double[_nChannelsOut];
	//memset(_pClippingChannels, 0, _nChannelsOut * sizeof(double));

	//Initialize conditions
	//_pUsedChannels = new bool[_nChannelsIn];
	//memset(_pUsedChannels, 0, _nChannelsIn * sizeof(bool));
	//Condition::init(_pUsedChannels);
}

void CaptureLoop::destroy() {
	for (int i = 0; i < NUM_BUFFERS; ++i) {
		_buffers[i].destroy();
	}
	_pInputs = nullptr;
	_pOutputs = nullptr;
}

void CaptureLoop::run() {
	_renderBufferCapacity = _pConfig->useAsioRenderer() ? AsioDevice::bufferSize : _pRenderDevice->getBufferSize();
	//Capture buffers
	for (int i = 0; i < NUM_BUFFERS; ++i) {
		_buffers[i].init(_renderBufferCapacity, _nChannelsOut);
	}
	_resetBuffers();
	_run = _silent = true;

	//Asio render device
	if (_pConfig->useAsioRenderer()) {
		AsioDevice::printInfo();
		ASIOCallbacks callbacks{ 0 };
		callbacks.bufferSwitch = &_bufferSwitch;
		//Asio driver automatically starts in new thread
		AsioDevice::startRenderService(&callbacks, (long)_nChannelsOut);
	}
	//Wasapi render device
	else {
		_pRenderDevice->startService();
		//Start rendering in a new thread.
		_wasapiRenderThread = std::thread(_wasapiRenderLoop);
	}

	_pCaptureDevice->startService();
	_wasapiCaptureThread = std::thread(_wasapiCaptureLoop);

	int count = 0;
	for (;;) {
		//Short sleep just to not busy wait all resources.
		Date::sleepMillis(100);

		//Check if config file has changed
		_checkConfig();

		//Dont do as often. Every 5seconds or so.
		if (count == 50) {
			count = 0;

			//Check if clipping has occured since last.
			_checkClippingChannels();

			//Update conditional routing requirements since last.
			_updateConditionalRouting();
		}
		++count;
	}
}

void CaptureLoop::stop() {
	if (_run) {
		_run = false;
		_wasapiCaptureThread.join();
		if (_pConfig->useAsioRenderer()) {
			AsioDevice::stopRenderService();
		}
		else {
			_wasapiRenderThread.join();
		}
	}
}

void fillWithSine(Buffer &buffer) {
	double *pBuffer = buffer.getData();
	for (size_t sampleIndex = 0; sampleIndex < buffer._capacity; ++sampleIndex) {
		static SineGenerator sinGen(44100, 300);
		const double val = sinGen.next();
		for (size_t channelIndex = 0; channelIndex < buffer._numChannels; ++channelIndex) {
			pBuffer[channelIndex] = val;
		}
		pBuffer += buffer._numChannels;
	}
	buffer.releaseData(buffer._capacity);
}

void CaptureLoop::_wasapiCaptureLoop() {
	OS::setPriorityHigh();
	UINT32 captureAvailable, captureLeft, renderAvailable, length, bufferIndex, sampleIndex, channelIndex;
	float *pCaptureBuffer = nullptr;
	double *pRenderBuffer;
	DWORD flags;
	bool silence = true;
	while (_run) {
		//Wait until its time to fill the next buffer.
		_waitSwap();

		bufferIndex = _getBufferIndexCapture();
		renderAvailable = _buffers[bufferIndex].getAvailableSize();

		//fillWithSine(_buffers[bufferIndex]);
		//continue;

		while (renderAvailable > 0) {
			//Check for samples in capture buffer.
			assert(_pCaptureDevice->getNextPacketSize(&captureAvailable));

			//No data and no silence. 
			if (captureAvailable == 0) {
				//Short sleep just to not busy wait all resources.
				Date::sleepMillis(1);
				continue;
			}

			//Silence flag set. 
			if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
				//Was audio before. Reset filter states.
				if (!silence) {
					silence = true;
					_setSilence(silence);
					_resetFilters();
				}
			}
			//Whent from silence to audio playing.
			else if (silence) {
				_resetBuffers();
				silence = false;
				_setSilence(silence);
			}

			if (silence) {
				assert(_pCaptureDevice->releaseCaptureBuffer(captureAvailable));
				//Short sleep just to not busy wait all resources.
				Date::sleepMillis(1);
				continue;
			}

			//Get capture buffer pointer and number of available frames.
			assert(_pCaptureDevice->getCaptureBuffer(&pCaptureBuffer, &captureAvailable, &flags));

			captureLeft = captureAvailable;

			while (captureLeft > 0) {

				//Available frames to write to buffer.
				length = min(captureLeft, renderAvailable);

				//Copy data from capture buffer to temporary render buffer.
				pRenderBuffer = _buffers[bufferIndex].getData();
				for (sampleIndex = 0; sampleIndex < length; ++sampleIndex) {
					for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
						//pBuffer[(bufferSize + sampleIndex) * _nChannelsOut + channelIndex] = pCaptureBuffer[sampleIndex * _nChannelsIn + channelIndex];
						pRenderBuffer[channelIndex] = pCaptureBuffer[channelIndex];
					}
					pCaptureBuffer += _nChannelsIn;
					pRenderBuffer += _nChannelsOut;
				}
				_buffers[bufferIndex].releaseData(length);

				captureLeft -= length;
				renderAvailable -= length;

				if (renderAvailable == 0) {
					bufferIndex = _getNextBufferIndex(bufferIndex);
					renderAvailable = _buffers[bufferIndex].getAvailableSize();
				}

			}

			assert(_pCaptureDevice->releaseCaptureBuffer(captureAvailable));
		}

	}




	////Check for samples in capture buffer.
	//assert(_pCaptureDevice->getNextPacketSize(&captureAvailable));

	////No data and no silence. 
	//if (captureAvailable == 0) {
	//	//Short sleep just to not busy wait all resources.
	//	Date::sleepMillis(1);
	//	continue;
	//}

	////Get capture buffer pointer and number of available frames.
	//assert(_pCaptureDevice->getCaptureBuffer(&pCaptureBuffer, &captureAvailable, &flags));

	////Silence flag set. 
	//if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
	//	//Was audio before. Reset filter states.
	//	if (!silence) {
	//		silence = true;
	//		_setSilence(silence);
	//		_resetFilters();
	//	}
	//}
	////Whent from silence to audio playing.
	//else if (silence) {
	//	_resetBuffers();
	//	silence = false;
	//	_setSilence(silence);
	//}

	//if (silence) {
	//	assert(_pCaptureDevice->releaseCaptureBuffer(captureAvailable));
	//	//Short sleep just to not busy wait all resources.
	//	Date::sleepMillis(1);
	//	continue;
	//}

	//captureLeft = captureAvailable;

	//while (captureLeft > 0) {
	//	//Available frames to write to buffer.
	//	length = min(captureLeft, renderAvailable);

	//	static SineGenerator sinGen(44100, 300);

	//	//Copy data from capture buffer to temporary render buffer.
	//	pBuffer = _buffers[bufferIndex].getData();
	//	for (sampleIndex = 0; sampleIndex < length; ++sampleIndex) {
	//		double val = sinGen.next();

	//		for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
	//			//pBuffer[(bufferSize + sampleIndex) * _nChannelsOut + channelIndex] = pCaptureBuffer[sampleIndex * _nChannelsIn + channelIndex];
	//			//pBuffer[channelIndex] = pCaptureBuffer[channelIndex];
	//			pBuffer[channelIndex] = val;
	//		}
	//		pCaptureBuffer += _nChannelsIn;
	//		pBuffer += _nChannelsOut;
	//	}
	//	_buffers[bufferIndex].releaseData(length);

	//	captureLeft -= length;
	//	renderAvailable -= length;

	//	if (renderAvailable == 0) {
	//		bufferIndex = _getNextBufferIndex(bufferIndex);
	//		renderAvailable = _buffers[bufferIndex].getAvailableSize();
	//	}

	//}

	//assert(_pCaptureDevice->releaseCaptureBuffer(captureAvailable));
//}
}

void CaptureLoop::_bufferSwitch(const long asioBufferIndex, const ASIOBool) {
	static size_t channelIndex, sampleIndex, bufferIndex;
	static int *pRenderBuffer;
	static double *pCaptureBuffer;

	//Silence. Just write 0 to render buffer.
	/*if (_isSilence()) {
		AsioDevice::renderSilence(asioBufferIndex);
		return;
	}*/

	bufferIndex = _getBufferIndexRender();
	pCaptureBuffer = _buffers[bufferIndex].getData();

	//Copy data from temporary render buffer to the real deal.
	for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
		pRenderBuffer = (int*)AsioDevice::pBufferInfos[channelIndex].buffers[asioBufferIndex];
		for (sampleIndex = 0; sampleIndex < _renderBufferCapacity; ++sampleIndex) {
			pRenderBuffer[sampleIndex] = (int)(MAX_INT32 * pCaptureBuffer[sampleIndex * _nChannelsOut + channelIndex]);
		}
	}
	_buffers[bufferIndex].releaseData();

	//for (sampleIndex = 0; sampleIndex < _renderBufferCapacity; ++sampleIndex) {
	//	for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
	//		pRenderBuffer = (int*)AsioDevice::pBufferInfos[channelIndex].buffers[asioBufferIndex];
	//		static SineGenerator sinGen(44100, 300);
	//		pRenderBuffer[sampleIndex] = (int)(MAX_INT32 * sinGen.next());
	//	}
	//}

	//Signal that we are done with this buffer.
	_advanceBufferIndex();

	//If available this can reduce letency.
	if (AsioDevice::outputReady) {
		assertAsio(ASIOOutputReady());
	}

	_notifySwap();
}

void CaptureLoop::_wasapiRenderLoop() {
	//OS::setPriorityHigh();
	//size_t channelIndex, sampleIndex, bufferIndex;
	//UINT32 numFramesAvailable;
	//float *pRenderBuffer;
	//double *pSource;
	//while (_run) {
	//	//Wait for device callback
	//	assertWait(WaitForSingleObjectEx(_pRenderDevice->getEventHandle(), INFINITE, false));

	//	//Get number of available framesd in the render buffer.
	//	numFramesAvailable = _pRenderDevice->getBufferFrameCountAvailable();

	//	//No space to render of nothinf plyaing. Just do nothing.
	//	if (numFramesAvailable || _isSilence()) {
	//		continue;
	//	}

	//	bufferIndex = _getBufferIndex();

	//	//Get render buffer
	//	assert(_pRenderDevice->getRenderBuffer(&pRenderBuffer, _renderBufferCapacity));

	//	//Copy data from temporary render buffer to the real deal.
	//	pSource = _buffers[bufferIndex].getData();
	//	for (sampleIndex = 0; sampleIndex < _renderBufferCapacity; ++sampleIndex) {
	//		for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
	//			pRenderBuffer[sampleIndex * _nChannelsOut + channelIndex] = (float)pSource[sampleIndex * _nChannelsOut + channelIndex];
	//		}
	//	}
	//	_buffers[bufferIndex].releaseData();

	//	//Signal that we are done with this buffer.
	//	_setNextBufferIndex(bufferIndex);

	//	//Release/flush render buffer.
	//	assert(_pRenderDevice->releaseRenderBuffer(_renderBufferCapacity));
	//}
}

void CaptureLoop::_writeToBuffer(const float* const pSource, const size_t bufferIndex, const size_t length, const size_t offset) {
	static size_t channelIndex, sampleIndex;
	static double *pBuffer;
	pBuffer = _buffers[bufferIndex].getData();
	for (sampleIndex = 0; sampleIndex < length; ++sampleIndex) {
		for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
			pBuffer[sampleIndex * _nChannelsOut + channelIndex] = pSource[sampleIndex * _nChannelsIn + channelIndex];
		}
	}
	_buffers[bufferIndex].releaseData(length);
}


//_captureBuffer.write(pCaptureBuffer, numFramesAvailable * _nChannelsOut);

			////Iterate all capture frames
			//for (sampleIndex = 0; sampleIndex < length; ++sampleIndex) {
			//	//Set default value to 0 so we can add/mix values to it later
			//	memset(_renderBlockBuffer, 0, _renderBlockSize);

			//	//Iterate inputs
			//	for (channelIndex = 0; channelIndex < _nChannelsIn; ++channelIndex) {
			//		////Identify which channels are playing.
			//		//if (pCaptureBuffer[j] != 0) {
			//		//	_pUsedChannels[j] = true;
			//		//}
			//		//Route sample to outputs
			//		(*_pInputs)[channelIndex]->route(pCaptureBuffer[channelIndex], _renderBlockBuffer);
			//	}

			//	//Iterate outputs
			//	for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
			//		int *pRenderBuffer = (int*)AsioDevice::pBufferInfos[channelIndex].buffers[bufferIndex];
			//		//Apply output forks and filters
			//		pRenderBuffer[offset + sampleIndex] = (int)(MAX_INT32 * _renderBlockBuffer[channelIndex]);
			//		//pRenderBuffer[offset + sampleIndex] = (int)(MAX_INT32 * (*_pOutputs)[channelIndex]->process(_renderBlockBuffer[channelIndex]));
			//	}

			//	//Move buffers to next sample
			//	pCaptureBuffer += _nChannelsIn;
			//	///pRenderBuffer += _nChannelsOut;
			//}


//void CaptureLoop::_bufferSwitch(const long bufferIndex, const ASIOBool) {
//	static UINT32 numFramesAvailable, length, framesLeft, sampleIndex, channelIndex, offset;
//	static float *pCaptureBuffer = nullptr;
//	static DWORD flags;
//
//	framesLeft = AsioDevice::bufferSize;
//
//	//Have data leftover from last call. Process these first.
//	if (_overflowSize > 0) {
//		length = min(_overflowSize, framesLeft);
//		_writeToBuffer(_pOverflowBuffer, length, bufferIndex);
//		_overflowSize -= length;
//		framesLeft -= length;
//	}
//
//	//Must fill entire render buffer before returning from func.
//	while (framesLeft > 0) {
//		//Check for samples in capture buffer.
//		assert(_pCaptureDevice->getNextPacketSize(&numFramesAvailable));
//
//		//No data in capture device. Just return.
//		if (numFramesAvailable == 0) {
//			//Short sleep just to not busy wait all resources.
//			Sleep(1);
//			continue;
//		}
//
//		//Get capture buffer pointer and number of available frames.
//		assert(_pCaptureDevice->getCaptureBuffer(&pCaptureBuffer, &numFramesAvailable, &flags));
//
//		//Silence. Do NOT send anything to render buffer.
//		if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
//			//Was audio before. Reset filter states.
//			if (!_silent) {
//				_silent = true;
//				_resetFilters();
//			}
//			//Silence. Just write silence and return to swap buffer.
//			AsioDevice::renderSilence(bufferIndex);
//			framesLeft = numFramesAvailable = length = 0;
//		}
//		//Audio playing. Send to render buffer.
//		else {
//			_silent = false;
//
//			offset = AsioDevice::bufferSize - framesLeft;
//
//			//Write samples to render buffer.
//			length = min(numFramesAvailable, framesLeft);
//
//			////Iterate all capture frames
//			//for (sampleIndex = 0; sampleIndex < length; ++sampleIndex) {
//			//	//Set default value to 0 so we can add/mix values to it later
//			//	memset(_renderBlockBuffer, 0, _renderBlockSize);
//
//			//	//Iterate inputs
//			//	for (channelIndex = 0; channelIndex < _nChannelsIn; ++channelIndex) {
//			//		////Identify which channels are playing.
//			//		//if (pCaptureBuffer[j] != 0) {
//			//		//	_pUsedChannels[j] = true;
//			//		//}
//			//		//Route sample to outputs
//			//		(*_pInputs)[channelIndex]->route(pCaptureBuffer[channelIndex], _renderBlockBuffer);
//			//	}
//
//			//	//Iterate outputs
//			//	for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
//			//		int *pRenderBuffer = (int*)AsioDevice::pBufferInfos[channelIndex].buffers[bufferIndex];
//			//		//Apply output forks and filters
//			//		pRenderBuffer[offset + sampleIndex] = (int)(MAX_INT32 * _renderBlockBuffer[channelIndex]);
//			//		//pRenderBuffer[offset + sampleIndex] = (int)(MAX_INT32 * (*_pOutputs)[channelIndex]->process(_renderBlockBuffer[channelIndex]));
//			//	}
//
//			//	//Move buffers to next sample
//			//	pCaptureBuffer += _nChannelsIn;
//			//	///pRenderBuffer += _nChannelsOut;
//			//}
//
//			//_writeToBuffer(pCaptureBuffer, length, bufferIndex, AsioDevice::bufferSize - framesLeft);
//
//			framesLeft -= length;
//		}
//
//		//Release/flush capture buffer.
//		assert(_pCaptureDevice->releaseCaptureBuffer(numFramesAvailable));
//	}
//
//	//Data left in capture buffer. Store in overflow.
//	if (numFramesAvailable > length) {
//		_overflowSize = numFramesAvailable - length;
//		//memcpy(_pOverflowBuffer, pCaptureBuffer + length * _nChannelsIn, _overflowSize * _nChannelsIn * sizeof(float));
//		memcpy(_pOverflowBuffer, pCaptureBuffer, _overflowSize * _nChannelsIn * sizeof(float));
//	}
//
//	//If available this can reduce letency.
//	if (AsioDevice::outputReady) {
//		assertAsio(ASIOOutputReady());
//	}
//}

//void CaptureLoop::_writeToBuffer(const float* const pSource, const UINT32 length, const int bufferIndex, const UINT32 offset) {
//	static size_t channelIndex, sampleIndex;
//	for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
//		int *pRenderBuffer = (int*)AsioDevice::pBufferInfos[channelIndex].buffers[bufferIndex];
//		for (sampleIndex = 0; sampleIndex < length; ++sampleIndex) {
//			pRenderBuffer[offset + sampleIndex] = (int)(MAX_INT32 * pSource[sampleIndex * _nChannelsIn + channelIndex]);
//		}
//	}
//}

//void CaptureLoop::_wasapiLoop() {
//	OS::setPriorityHigh();
//	UINT32 i, j, numFramesAvailable;
//	float *pRenderBuffer;
//	bool silenceToPlayback;
//	while (_run) {
//		//Get capture data.
//		_getCaptureBuffer(&numFramesAvailable, &silenceToPlayback);
//
//		//No data to render. Just continue and wait for next iteration.
//		if (_silent || numFramesAvailable == 0) {
//			Sleep(1);
//			continue;
//		}
//
//		//Was silent before. Needs to flush render buffer before adding new data.
//		if (silenceToPlayback) {
//			_pRenderDevice->flushRenderBuffer();
//		}
//
//		//Must read entire capture buffer at once. Wait until render buffer has enough space available.
//		while (numFramesAvailable > _pRenderDevice->getBufferFrameCountAvailable()) {
//			//Short sleep just to not busy wait all resources.
//			Sleep(1);
//		}
//		
//		//Get render buffer
//		assert(_pRenderDevice->getRenderBuffer(&pRenderBuffer, numFramesAvailable));
//
//		//Copy frames from tmp render buffer to actual render buffer.
//		for (i = 0; i < numFramesAvailable; ++i) {
//			for (j = 0; j < _nChannelsOut; ++j) {
//				pRenderBuffer[i * _nChannelsOut + j] = (float)_renderTmpBuffer[i * _nChannelsOut + j];
//			}
//		}
//
//		//Release/flush render buffer.
//		assert(_pRenderDevice->releaseRenderBuffer(numFramesAvailable));
//	}
//}

//void CaptureLoop::_wasapiRenderLoop() {
//	//OS::setPriorityHigh();
//	//UINT32 i, j, numFramesAvailable;
//	//float *pRenderBuffer;
//	//bool silenceToPlayback;
//	//while (_run) {
//	//	//Wait for device callback
//	//	assertWait(WaitForSingleObjectEx(_pRenderDevice->getEventHandle(), INFINITE, false));
//
//	//	//Get capture data.
//	//	_getCaptureBuffer(&numFramesAvailable, &silenceToPlayback);
//
//	//	//No data to render. Just continue and wait for next iteration.
//	//	if (_silent || numFramesAvailable == 0) {
//	//		continue;
//	//	}
//
//	//	//Was silent before. Needs to flush render buffer before adding new data.
//	//	if (silenceToPlayback) {
//	//		_pRenderDevice->flushRenderBuffer();
//	//	}
//
//	//	//Must read entire capture buffer at once. Wait until render buffer has enough space available.
//	//	while (numFramesAvailable > _pRenderDevice->getBufferFrameCountAvailable()) {
//	//		//Short sleep just to not busy wait all resources.
//	//		Sleep(1);
//	//	}
//
//	//	//Get render buffer
//	//	assert(_pRenderDevice->getRenderBuffer(&pRenderBuffer, numFramesAvailable));
//
//	//	////Copy frames from tmp render buffer to actual render buffer.
//	//	//for (i = 0; i < numFramesAvailable; ++i) {
//	//	//	for (j = 0; j < _nChannelsOut; ++j) {
//	//	//		pRenderBuffer[i * _nChannelsOut + j] = (float)_renderTmpBuffer[i * _nChannelsOut + j];
//	//	//	}
//	//	//}
//
//	//	//Release/flush render buffer.
//	//	assert(_pRenderDevice->releaseRenderBuffer(numFramesAvailable));
//	//}
//}

//void CaptureLoop::_bufferSwitch(const long bufferIndex, const ASIOBool) {
//	static UINT32 numFramesAvailable, length, framesLeft;
//	framesLeft = AsioDevice::bufferSize;
//
//	//Have data leftover from last call. Process these first.
//	if (_overflowSize > 0) {
//		length = min(_overflowSize, framesLeft);
//		_writeToBuffer(_pOverflowBuffer, length, bufferIndex);
//		_overflowSize -= length;
//		framesLeft -= length;
//	}
//
//	//Must fill entire render buffer before returning from func.
//	while (framesLeft > 0) {
//		//Get capture data.
//		_getCaptureBuffer(&numFramesAvailable);
//
//		//Silence. Just write silence and return to swap buffer.
//		if (_silent) {
//			AsioDevice::renderSilence(bufferIndex);
//			return;
//		}
//
//		//No data in capture device right now. Just continue and check again.
//		if (numFramesAvailable == 0) {
//			//Short sleep just to not busy wait all resources.
//			Sleep(1);
//			continue;
//		}
//
//		length = min(numFramesAvailable, framesLeft);
//
//		//memset(_renderTmpBuffer, 0, _renderBlockSize * AsioDevice::bufferSize);
//
//		//Write samples to render buffer.
//		_writeToBuffer(_renderTmpBuffer, length, bufferIndex, AsioDevice::bufferSize - framesLeft);
//
//		framesLeft -= length;
//	}
//
//	//Data left in capture buffer. Store in overflow.
//	if (numFramesAvailable > length) {
//		_overflowSize = numFramesAvailable - length;
//		memcpy(_pOverflowBuffer, _renderTmpBuffer + length * _nChannelsOut, _overflowSize * _nChannelsOut * sizeof(double));
//	}
//
//	//If available this can reduce letency.
//	if (AsioDevice::outputReady) {
//		assertAsio(ASIOOutputReady());
//	}
//}

//void CaptureLoop::_writeToBuffer(const double* const pSource, const UINT32 length, const int bufferIndex, const UINT32 offset) {
//	for (size_t channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
//		int *pRenderBuffer = pRenderBuffer = (int*)AsioDevice::pBufferInfos[channelIndex].buffers[bufferIndex];
//		for (size_t sampleIndex = 0; sampleIndex < length; ++sampleIndex) {
//			pRenderBuffer[offset + sampleIndex] = (int)(MAX_INT32 * pSource[sampleIndex * _nChannelsOut + channelIndex]);
//		}
//	}
//}

//void CaptureLoop::_getCaptureBuffer(UINT32 * const pNumFramesAvailable, bool * const pSilenceToPlayback) {
//	//Check for samples in capture buffer.
//	assert(_pCaptureDevice->getNextPacketSize(pNumFramesAvailable));
//
//	//No data in capture device. Just return.
//	if (*pNumFramesAvailable == 0) {
//		return;
//	}
//
//	//Get capture buffer pointer and number of available frames.
//	static float *pCaptureBuffer;
//	static DWORD flags;
//	assert(_pCaptureDevice->getCaptureBuffer(&pCaptureBuffer, pNumFramesAvailable, &flags));
//
//	//Silence. Do NOT send anything to render buffer.
//	if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
//		//Was audio before. Reset filter states.
//		if (!_silent) {
//			_silent = true;
//			_resetFilters();
//		}
//	}
//	//Audio playing. Send to render buffer.
//	else {
//		if (pSilenceToPlayback) {
//			//Is true if it was silent before
//			*pSilenceToPlayback = _silent;
//		}
//		_silent = false;
//
//		//Parse input data, apply routes and filters and fill temporary render buffer.
//		_fillRenderTmpBuffer(pCaptureBuffer, *pNumFramesAvailable);
//	}
//
//	//Release/flush capture buffer.
//	assert(_pCaptureDevice->releaseCaptureBuffer(*pNumFramesAvailable));
//}

//void CaptureLoop::_fillRenderTmpBuffer(float* captureBuffer, const UINT32 length) {
//	//Set default value to 0 so we can add/mix values to it later
//	memset(_renderTmpBuffer, 0, _renderBlockSize * length);
//
//	//Iterate all capture frames
//	static double* renderBuffer;;
//	renderBuffer = _renderTmpBuffer;
//	static UINT32 i, j;
//	for (i = 0; i < length; ++i) {
//		//Iterate inputs
//		for (j = 0; j < _nChannelsIn; ++j) {
//			//Identify which channels are playing.
//			if (captureBuffer[j] != 0.0f) {
//				_pUsedChannels[j] = true;
//			}
//			//Route sample to outputs
//			(*_pInputs)[j]->route(captureBuffer[j], renderBuffer);
//		}
//
//		//Iterate outputs
//		for (j = 0; j < _nChannelsOut; ++j) {
//			//Apply output forks and filters
//			renderBuffer[j] = (*_pOutputs)[j]->process(renderBuffer[j]);
//
//			//Check for clipping
//			if (abs(renderBuffer[j]) > 1.0) {
//				_clippingDetected = true;
//				_pClippingChannels[j] = max(_pClippingChannels[j], abs(renderBuffer[j]));
//			}
//		}
//
//		//Move buffers to next sample
//		captureBuffer += _nChannelsIn;
//		renderBuffer += _nChannelsOut;
//	}
//}

void CaptureLoop::_resetFilters() {
	//Reset i/o filter states.
	for (Input *p : *_pInputs) {
		p->reset();
	}
	for (Output *p : *_pOutputs) {
		p->reset();
	}
}

void CaptureLoop::_checkConfig() {
	//Check if new config file has been selected
	const char input = Keyboard::getDigit();
	if (input) {
		throw ConfigChangedException(input);
	}
	//Check if config file on disk has changed
	if (_pConfig->hasChanged()) {
		throw ConfigChangedException();
	}
}

void CaptureLoop::_checkClippingChannels() {
	//for (size_t i = 0; i < _nChannelsOut; ++i) {
	//	if (_pClippingChannels[i] != 0) {
	//		printf("WARNING: Output(%s) - Clipping detected: +%0.2f dBFS\n", _pConfig->getChannelName(i).c_str(), Convert::levelToDb(_pClippingChannels[i]));
	//		_pClippingChannels[i] = 0;
	//	}
	//}
}

void CaptureLoop::_updateConditionalRouting() {
	//Update conditional routing.
	for (const Input *pInut : *_pInputs) {
		pInut->evalConditions();
	}
	//Set all to false so we can check anew until next time this func runs.
	/*for (size_t i = 0; i < _nChannelsIn; ++i) {
		_pUsedChannels[i] = false;
	}*/
}

//For debug purposes only.
void CaptureLoop::_printUsedChannels() {
	/*for (size_t i = 0; i < _nChannelsIn; ++i) {
		printf("%s %d\n", _pConfig->getChannelName(i).c_str(), _pUsedChannels[i]);
	}
	printf("\n");*/
}



/*	if (bufferSize != _renderBufferCapacity) {
		pBuffer = _buffers[bufferIndex].getData();
		for (sampleIndex = 0; sampleIndex < _renderBufferCapacity; ++sampleIndex) {
			double val = sinGen.next();
			for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
				pBuffer[sampleIndex * _nChannelsOut + channelIndex] = val;
			}
		}
		_buffers[bufferIndex].releaseData(_renderBufferCapacity);
	}
	else {
		Sleep(1);
	}*/


	//static SineGenerator sinGen(44100, 300);
	//double *pBuffer = _buffers[bufferIndex].getData();
	//for (sampleIndex = 0; sampleIndex < _renderBufferCapacity; ++sampleIndex) {
	//	double val = sinGen.next();
	//	for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
	//		pBuffer[sampleIndex * _nChannelsOut + channelIndex] = val;
	//	}
	//}
	//_buffers[bufferIndex].releaseData(_renderBufferCapacity);
	//_buffers[bufferIndex].resetSize();
	//#ifdef DEBUG
		//			if (flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) {
		//				printf("AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY\n");
		//			}
		//			if (flags & AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR) {
		//				printf("AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR\n");
		//			}
		//#endif
