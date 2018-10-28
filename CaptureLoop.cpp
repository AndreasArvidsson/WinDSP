#include "CaptureLoop.h"
#include "AsioDevice.h"
#include "OS.h"

//TODO
#include "Stopwatch.h"
Stopwatch swCap("Capture", 500);
Stopwatch swRen("Render", 500);

#define MAX_INT32 2147483648.0

const Config *CaptureLoop::_pConfig;
const std::vector<Input*> *CaptureLoop::_pInputs = nullptr;
const std::vector<Output*> *CaptureLoop::_pOutputs = nullptr;
AudioDevice *CaptureLoop::_pCaptureDevice = nullptr;
AudioDevice *CaptureLoop::_pRenderDevice = nullptr;
size_t CaptureLoop::_nChannelsIn = 0;
size_t CaptureLoop::_nChannelsOut = 0;
size_t CaptureLoop::_renderBlockSize = 0;
UINT32 CaptureLoop::_renderBufferCapacity = 0;
std::thread CaptureLoop::_wasapiCaptureThread;
std::thread CaptureLoop::_wasapiRenderThread;
Buffer CaptureLoop::_buffers[NUM_BUFFERS];
std::atomic<bool> CaptureLoop::_run = false;
size_t  CaptureLoop::_bufferIndexRender = 0;

std::condition_variable CaptureLoop::_swapCondition;
Semaphore CaptureLoop::_swap(0, 1);

void CaptureLoop::init(const Config *pConfig, AudioDevice *pCaptureDevice, AudioDevice *pRenderDevice) {
	_pConfig = pConfig;
	_pInputs = pConfig->getInputs();
	_pOutputs = pConfig->getOutputs();
	_pCaptureDevice = pCaptureDevice;
	_pRenderDevice = pRenderDevice;
	_nChannelsIn = _pInputs->size();
	_nChannelsOut = _pOutputs->size();

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
	_bufferIndexRender = 0;
	_buffers[0].fillWithSilence();
	_run = true;

	_pCaptureDevice->startService();
	_wasapiCaptureThread = std::thread(_wasapiCaptureLoop);

	//Asio render device
	if (_pConfig->useAsioRenderer()) {
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

	size_t count = 0;
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

void CaptureLoop::processCaptureBuffer(const float *pCaptureBuffer, size_t captureAvailable, size_t bufferIndex) {
	size_t length, renderAvailable;
	double *pRenderBuffer;
	while (captureAvailable > 0) {
		renderAvailable = _buffers[bufferIndex].getAvailableSize();

		//Available frames to write to buffer.
		length = min(captureAvailable, renderAvailable);

		//_buffers[bufferIndex].lock();

		//Copy data from capture buffer to temporary render buffer.
		pRenderBuffer = _buffers[bufferIndex].getDataWithOffset();
		for (size_t sampleIndex = 0; sampleIndex < length; ++sampleIndex) {
			for (size_t channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
				pRenderBuffer[channelIndex] = pCaptureBuffer[channelIndex];
			}
			pCaptureBuffer += _nChannelsIn;
			pRenderBuffer += _nChannelsOut;
		}
		_buffers[bufferIndex].releaseData(length);

		//_buffers[bufferIndex].unlock();

		captureAvailable -= length;
		renderAvailable -= length;

		if (captureAvailable > 0 && renderAvailable == 0) {
			bufferIndex = _getNextBufferIndex(bufferIndex);
#ifdef DEBUG
			if (renderAvailable > _buffers[bufferIndex].getAvailableSize()) {
				printf("To small buffer %zd, %zd, %zd\n", bufferIndex, renderAvailable, _buffers[bufferIndex].getAvailableSize());
			}
#endif
		}
	}
}

void CaptureLoop::_wasapiCaptureLoop() {
	OS::setPriorityHigh();
	size_t renderAvailable;
	UINT32 captureAvailable;
	float *pCaptureBuffer;
	DWORD flags;
	bool silence = true;
	size_t bufferIndex = 1;
	while (_run) {
		//Wait until its time to fill the next buffer.
		_swap.wait();

		swCap.intervalStart();

		renderAvailable = _buffers[bufferIndex].getAvailableSize();

		//Must fill entire render buffer in this one event/call.
		while (renderAvailable > 0) {
			//Check for samples in capture buffer.
			assert(_pCaptureDevice->getNextPacketSize(&captureAvailable));

			//No data available
			if (captureAvailable == 0) {
				//In silence. Fill buffer with silence and be done.
				if (silence) {
					_buffers[bufferIndex].fillWithSilence();
					break;
				}
				//Just waiting for data.
				else {
					//Short sleep just to not busy wait all resources.
					Date::sleepMillis(1);
					continue;
				}
			}

			//Get capture buffer pointer and number of available frames.
			assert(_pCaptureDevice->getCaptureBuffer(&pCaptureBuffer, &captureAvailable, &flags));

			//Silence flag set. 
			if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
				//Was audio before. Reset filter states.
				if (!silence) {
					silence = true;
					_resetFilters();
				}
				_buffers[bufferIndex].fillWithSilence();
			}
			//Whent from silence to audio playing.
			else {
				silence = false;
				//We have data in the capture buffer. Process it.
				processCaptureBuffer(pCaptureBuffer, (size_t)captureAvailable, bufferIndex);
				renderAvailable -= min(renderAvailable, captureAvailable);
			}

			//Release/flush capture buffer.
			assert(_pCaptureDevice->releaseCaptureBuffer(captureAvailable));
		}

		//We are done with this buffer. Go to next.
		bufferIndex = _getNextBufferIndex(bufferIndex);

		swCap.intervalEnd();
	}
}

void CaptureLoop::_bufferSwitch(const long asioBufferIndex, const ASIOBool) {
	static size_t channelIndex, sampleIndex;
	static int *pRenderBuffer;
	static double *pCaptureBuffer;

	swRen.intervalStart();

	//Notify capture thread that this renderer is processing.
	_swap.notify();

#ifdef DEBUG 
	if (_buffers[_bufferIndexRender].size() != _renderBufferCapacity) {
		printf("Render size missmatch %d %zd\n", _renderBufferCapacity, _buffers[_bufferIndexRender].size());
		//return;
	}
#endif

	//_buffers[_bufferIndexRender].lock();

	//Copy data from temporary render buffer to the real deal.
	pCaptureBuffer = _buffers[_bufferIndexRender].getData();
	for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
		pRenderBuffer = (int*)AsioDevice::pBufferInfos[channelIndex].buffers[asioBufferIndex];
		for (sampleIndex = 0; sampleIndex < _renderBufferCapacity; ++sampleIndex) {
			pRenderBuffer[sampleIndex] = (int)(MAX_INT32 * pCaptureBuffer[sampleIndex * _nChannelsOut + channelIndex]);
		}
	}
	_buffers[_bufferIndexRender].releaseData();

	//_buffers[_bufferIndexRender].unlock();

	//We are done with this buffer. Go to next.
	_bufferIndexRender = _getNextBufferIndex(_bufferIndexRender);

	//If available this can reduce letency.
	if (AsioDevice::outputReady) {
		assertAsio(ASIOOutputReady());
	}

	swRen.intervalEnd();
}

void CaptureLoop::_wasapiRenderLoop() {
	OS::setPriorityHigh();
	size_t channelIndex, sampleIndex;
	UINT32 numFramesAvailable;
	float *pRenderBuffer;
	double *pCaptureBuffer;
	while (_run) {
		//Wait for device callback
		assertWait(WaitForSingleObjectEx(_pRenderDevice->getEventHandle(), INFINITE, false));

		swRen.intervalStart();

		//Notify capture thread that this renderer is processing.
		_swap.notify();

		//Get number of available framesd in the render buffer.
		numFramesAvailable = _pRenderDevice->getBufferFrameCountAvailable();

		//No space to render. Just ignore.
		if (numFramesAvailable > 0) {
			//No need to write silence to wasapi endpoint. Just leave it alone.

#ifdef DEBUG
			if (numFramesAvailable != _renderBufferCapacity) {
				printf("Render size missmatch %d %d\n", _renderBufferCapacity, numFramesAvailable);
			}
#endif
			//Get render buffer
			assert(_pRenderDevice->getRenderBuffer(&pRenderBuffer, _renderBufferCapacity));

			//Copy data from temporary render buffer to the real deal.
			pCaptureBuffer = _buffers[_bufferIndexRender].getData();
			for (sampleIndex = 0; sampleIndex < _renderBufferCapacity; ++sampleIndex) {
				for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
					pRenderBuffer[sampleIndex * _nChannelsOut + channelIndex] = (float)pCaptureBuffer[sampleIndex * _nChannelsOut + channelIndex];
				}
			}
			_buffers[_bufferIndexRender].releaseData();

			//Release/flush render buffer.
			assert(_pRenderDevice->releaseRenderBuffer(_renderBufferCapacity));
		}

		//We are done with this buffer. Go to next.
		_bufferIndexRender = _getNextBufferIndex(_bufferIndexRender);

		swRen.intervalEnd();
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

void CaptureLoop::_resetFilters() {
	//Reset i/o filter states.
	for (Input *p : *_pInputs) {
		p->reset();
	}
	for (Output *p : *_pOutputs) {
		p->reset();
	}
}

const size_t CaptureLoop::_getNextBufferIndex(const size_t index) {
	return index + 1 < NUM_BUFFERS ? index + 1 : 0;
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
	for (Output *pOutput : *_pOutputs) {
		if (pOutput->clipping != 0.0) {
			printf("WARNING: Output(%s) - Clipping detected: +%0.2f dBFS\n", pOutput->name.c_str(), Convert::levelToDb(pOutput->clipping));
			pOutput->clipping = 0.0;
		}
	}
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


//inline void writeToBuffer(float *pCaptureBuffer, Buffer &buf, int length, int offset) {
//	//Copy data from capture buffer to temporary render buffer.
//	static double* pRenderBuffer;
//	static int _nChannelsIn = 8;
//	static size_t sampleIndex, channelIndex;
//	pRenderBuffer = buf.getDataWithOffset();
//	for (sampleIndex = 0; sampleIndex < length; ++sampleIndex) {
//		for (channelIndex = 0; channelIndex < buf.numChannels(); ++channelIndex) {
//			//pBuffer[(bufferSize + sampleIndex) * _nChannelsOut + channelIndex] = pCaptureBuffer[sampleIndex * _nChannelsIn + channelIndex];
//			pRenderBuffer[channelIndex] = pCaptureBuffer[channelIndex];
//		}
//		pCaptureBuffer += _nChannelsIn;
//		pRenderBuffer += buf.numChannels();
//	}
//	buf.releaseData(length);
//	int a = 2;
//}

//void CaptureLoop::_wasapiCaptureLoop() {
//	OS::setPriorityHigh();
//	size_t captureLeft, renderAvailable, length, bufferIndex, sampleIndex, channelIndex, overflowSize;
//	UINT32 captureAvailable;
//	float *pCaptureBuffer = nullptr;
//	double *pRenderBuffer;
//	DWORD flags;
//	bool silence = true;
//
//	size_t renderLeft;
//	UINT32 numSamplesAvailable;
//
//	while (_run) {
//		//Wait until its time to fill the next buffer.
//		_swap.wait();
//
//		sw.intervalStart();
//
//		bufferIndex = _bufferIndexCapture;
//		renderLeft = _buffers[bufferIndex].getAvailableSize();
//
//		while (renderLeft > 0) {
//
//			//Check for samples in capture buffer.
//			assert(_pCaptureDevice->getNextPacketSize(&numSamplesAvailable));
//
//			//No data and no silence. 
//			if (numSamplesAvailable == 0) {
//				//Short sleep just to not busy wait all resources.
//				Date::sleepMillis(1);
//				continue;
//			}
//
//			//Get capture buffer pointer and number of available frames.
//			DWORD flags;
//			assert(_pCaptureDevice->getCaptureBuffer(&pCaptureBuffer, &numSamplesAvailable, &flags));
//
//			length = min(renderLeft, numSamplesAvailable);
//
//			//writeToBuffer(pCaptureBuffer, _buffers[bufferIndex], length, offset);
//
//			//Copy data from capture buffer to temporary render buffer.
//			pRenderBuffer = _buffers[bufferIndex].getDataWithOffset();
//			for (sampleIndex = 0; sampleIndex < length; ++sampleIndex) {
//				for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
//					pRenderBuffer[channelIndex] = pCaptureBuffer[channelIndex];
//				}
//				pCaptureBuffer += _nChannelsIn;
//				pRenderBuffer += _nChannelsOut;
//			}
//			_buffers[bufferIndex].releaseData(length);
//			renderLeft -= length;
//
//			if (numSamplesAvailable > length) {
//				overflowSize = numSamplesAvailable - length;
//
//				bufferIndex = _getNextBufferIndex(bufferIndex);
//				pRenderBuffer = _buffers[bufferIndex].getDataWithOffset();
//				for (sampleIndex = 0; sampleIndex < overflowSize; ++sampleIndex) {
//					for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
//						pRenderBuffer[channelIndex] = pCaptureBuffer[channelIndex];
//					}
//					pCaptureBuffer += _nChannelsIn;
//					pRenderBuffer += _nChannelsOut;
//				}
//				_buffers[bufferIndex].releaseData(overflowSize);
//				renderLeft -= overflowSize;
//			}
//
//			assert(_pCaptureDevice->releaseCaptureBuffer(numSamplesAvailable));
//		}
//
//		//We are done with this buffer. Go to next.
//		_bufferIndexCapture = _getNextBufferIndex(_bufferIndexCapture);
//
//		sw.intervalEnd();
//	}
//}

//#ifdef DEBUG
	//			if (flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) {
	//				printf("AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY\n");
	//			}
	//			if (flags & AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR) {
	//				printf("AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR\n");
	//			}
	//#endif


//#include "SineGenerator.h"
//void fillWithSine(Buffer &buffer) {
//	double *pBuffer = buffer.getData();
//	for (size_t sampleIndex = 0; sampleIndex < buffer._capacity; ++sampleIndex) {
//		static SineGenerator sinGen(44100, 300);
//		const double val = sinGen.next();
//		for (size_t channelIndex = 0; channelIndex < buffer._numChannels; ++channelIndex) {
//			pBuffer[channelIndex] = val;
//		}
//		pBuffer += buffer._numChannels;
//	}
//	buffer.releaseData(buffer._capacity);
//}
//fillWithSine(_buffers[bufferIndex]);
	//continue;






//void CaptureLoop::writeToBuffer(float *pCaptureBuffer, long asioBufferIndex, const size_t length, const size_t offset) {
//	static size_t channelIndex, sampleIndex;
//	static int *pRenderBuffer;
//	for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
//		pRenderBuffer = (int*)AsioDevice::pBufferInfos[channelIndex].buffers[asioBufferIndex];
//		for (sampleIndex = 0; sampleIndex < length; ++sampleIndex) {
//			pRenderBuffer[offset + sampleIndex] = (int)(MAX_INT32 * pCaptureBuffer[sampleIndex * _nChannelsIn + channelIndex]);
//		}
//	}
//}
//
//void CaptureLoop::_bufferSwitch(const long asioBufferIndex, const ASIOBool) {
//	static size_t channelIndex, sampleIndex;
//	static int *pRenderBuffer;
//	static double *pCaptureBuffer;
//	static float overflow[8000];
//
//	static float *pBuf;
//	static UINT32 numSamplesAvailable, renderLeft, length, offset;
//	static UINT32 overflowSize = 0;
//
//	sw.intervalStart();
//
//	renderLeft = _renderBufferCapacity;
//	offset = 0;
//
//	if (overflowSize) {
//		length = min(renderLeft, overflowSize);
//		writeToBuffer(overflow, asioBufferIndex, length, 0);
//		overflowSize = 0;
//		renderLeft -= length;
//		offset += length;
//	}
//
//	while (renderLeft > 0) {
//		//Check for samples in capture buffer.
//		assert(_pCaptureDevice->getNextPacketSize(&numSamplesAvailable));
//
//		//No data and no silence. 
//		if (numSamplesAvailable == 0) {
//			//Short sleep just to not busy wait all resources.
//			Date::sleepMillis(1);
//			continue;
//		}
//
//		//Get capture buffer pointer and number of available frames.
//		DWORD flags;
//		assert(_pCaptureDevice->getCaptureBuffer(&pBuf, &numSamplesAvailable, &flags));
//
//		length = min(renderLeft, numSamplesAvailable);
//
//		writeToBuffer(pBuf, asioBufferIndex, length, offset);
//
//		if (numSamplesAvailable > length) {
//			overflowSize = numSamplesAvailable - length;
//			memcpy(overflow, pBuf + length * _nChannelsIn, overflowSize * _nChannelsIn);
//		}
//
//		assert(_pCaptureDevice->releaseCaptureBuffer(numSamplesAvailable));
//
//		renderLeft -= length;
//		offset += length;
//	}
//
//	sw.intervalEnd();
//
//	//If available this can reduce letency.
//	if (AsioDevice::outputReady) {
//		assertAsio(ASIOOutputReady());
//	}
//}

//void CaptureLoop::_wasapiCaptureLoop() {
//	OS::setPriorityHigh();
//	size_t captureLeft, renderAvailable, length, bufferIndex, sampleIndex, channelIndex;
//	UINT32 captureAvailable;
//	float *pCaptureBuffer = nullptr;
//	double *pRenderBuffer;
//	DWORD flags;
//	bool silence = true;
//
//	float overflow[8000];
//	size_t offset, renderLeft;
//	UINT32 numSamplesAvailable;
//	size_t overflowSize = 0;
//
//	while (_run) {
//		//Wait until its time to fill the next buffer.
//		_swap.wait();
//
//		sw.intervalStart();
//
//		bufferIndex = _bufferIndexCapture;
//		/*	offset = _buffers[bufferIndex].size();
//			renderLeft = _renderBufferCapacity - offset;*/
//
//		renderLeft = _buffers[bufferIndex].getAvailableSize();
//
//		while (renderLeft > 0) {
//
//			if (overflowSize) {
//				length = min(renderLeft, overflowSize);
//				//writeToBuffer(overflow, _buffers[bufferIndex], length, offset);
//
//				pCaptureBuffer = overflow;
//				pRenderBuffer = _buffers[bufferIndex].getDataWithOffset();
//				for (sampleIndex = 0; sampleIndex < length; ++sampleIndex) {
//					for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
//						pRenderBuffer[channelIndex] = pCaptureBuffer[channelIndex];
//					}
//					pCaptureBuffer += _nChannelsIn;
//					pRenderBuffer += _nChannelsOut;
//				}
//				_buffers[bufferIndex].releaseData(length);
//
//				overflowSize -= length;
//				renderLeft -= length;
//				continue;
//			}
//
//			//Check for samples in capture buffer.
//			assert(_pCaptureDevice->getNextPacketSize(&numSamplesAvailable));
//
//			//No data and no silence. 
//			if (numSamplesAvailable == 0) {
//				//Short sleep just to not busy wait all resources.
//				Date::sleepMillis(1);
//				continue;
//			}
//
//			//Get capture buffer pointer and number of available frames.
//			DWORD flags;
//			assert(_pCaptureDevice->getCaptureBuffer(&pCaptureBuffer, &numSamplesAvailable, &flags));
//
//			length = min(renderLeft, numSamplesAvailable);
//
//			//writeToBuffer(pCaptureBuffer, _buffers[bufferIndex], length, offset);
//
//			//Copy data from capture buffer to temporary render buffer.
//			pRenderBuffer = _buffers[bufferIndex].getDataWithOffset();
//			for (sampleIndex = 0; sampleIndex < length; ++sampleIndex) {
//				for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
//					pRenderBuffer[channelIndex] = pCaptureBuffer[channelIndex];
//				}
//				pCaptureBuffer += _nChannelsIn;
//				pRenderBuffer += _nChannelsOut;
//			}
//			_buffers[bufferIndex].releaseData(length);
//
//			renderLeft -= length;
//
//			if (numSamplesAvailable > length) {
//				overflowSize = numSamplesAvailable - length;
//				memcpy(overflow, pCaptureBuffer, overflowSize * _nChannelsIn * sizeof(float));
//			}
//
//			assert(_pCaptureDevice->releaseCaptureBuffer(numSamplesAvailable));
//		}
//
//		//We are done with this buffer. Go to next.
//		_bufferIndexCapture = _getNextBufferIndex(_bufferIndexCapture);
//
//		sw.intervalEnd();
//	}
//}
//
//void CaptureLoop::_wasapiCaptureLoop() {
//	OS::setPriorityHigh();
//	size_t captureLeft, renderAvailable, length, bufferIndex, sampleIndex, channelIndex;
//	UINT32 captureAvailable;
//	float *pCaptureBuffer = nullptr;
//	double *pRenderBuffer;
//	DWORD flags;
//	bool silence = true;
//
//	float overflow[8000];
//	size_t offset, renderLeft;
//	UINT32 numSamplesAvailable;
//	size_t overflowSize = 0;
//
//	while (_run) {
//		//Wait until its time to fill the next buffer.
//		_swap.wait();
//
//		sw.intervalStart();
//
//		bufferIndex = _bufferIndexCapture;
//		/*	offset = _buffers[bufferIndex].size();
//			renderLeft = _renderBufferCapacity - offset;*/
//
//		renderLeft = _buffers[bufferIndex].getAvailableSize();
//
//		while (renderLeft > 0) {
//
//			if (overflowSize) {
//				length = min(renderLeft, overflowSize);
//				//writeToBuffer(overflow, _buffers[bufferIndex], length, offset);
//
//				pCaptureBuffer = overflow;
//				pRenderBuffer = _buffers[bufferIndex].getDataWithOffset();
//				for (sampleIndex = 0; sampleIndex < length; ++sampleIndex) {
//					for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
//						pRenderBuffer[channelIndex] = pCaptureBuffer[channelIndex];
//					}
//					pCaptureBuffer += _nChannelsIn;
//					pRenderBuffer += _nChannelsOut;
//				}
//				_buffers[bufferIndex].releaseData(length);
//
//				overflowSize -= length;
//				renderLeft -= length;
//				continue;
//			}
//
//			//Check for samples in capture buffer.
//			assert(_pCaptureDevice->getNextPacketSize(&numSamplesAvailable));
//
//			//No data and no silence. 
//			if (numSamplesAvailable == 0) {
//				//Short sleep just to not busy wait all resources.
//				Date::sleepMillis(1);
//				continue;
//			}
//
//			//Get capture buffer pointer and number of available frames.
//			DWORD flags;
//			assert(_pCaptureDevice->getCaptureBuffer(&pCaptureBuffer, &numSamplesAvailable, &flags));
//
//			length = min(renderLeft, numSamplesAvailable);
//
//			//writeToBuffer(pCaptureBuffer, _buffers[bufferIndex], length, offset);
//
//			//Copy data from capture buffer to temporary render buffer.
//			pRenderBuffer = _buffers[bufferIndex].getDataWithOffset();
//			for (sampleIndex = 0; sampleIndex < length; ++sampleIndex) {
//				for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
//					pRenderBuffer[channelIndex] = pCaptureBuffer[channelIndex];
//				}
//				pCaptureBuffer += _nChannelsIn;
//				pRenderBuffer += _nChannelsOut;
//			}
//			_buffers[bufferIndex].releaseData(length);
//
//			renderLeft -= length;
//
//			if (numSamplesAvailable > length) {
//				overflowSize = numSamplesAvailable - length;
//
//				bufferIndex = _getNextBufferIndex(bufferIndex);
//				pRenderBuffer = _buffers[bufferIndex].getDataWithOffset();
//				for (sampleIndex = 0; sampleIndex < overflowSize; ++sampleIndex) {
//					for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
//						pRenderBuffer[channelIndex] = pCaptureBuffer[channelIndex];
//					}
//					pCaptureBuffer += _nChannelsIn;
//					pRenderBuffer += _nChannelsOut;
//				}
//				_buffers[bufferIndex].releaseData(overflowSize);
//
//				renderLeft -= overflowSize;
//
//				/*	for (sampleIndex = 0; sampleIndex < overflowSize * _nChannelsIn; ++sampleIndex) {
//						overflow[sampleIndex] = pCaptureBuffer[sampleIndex];
//					}*/
//
//					//memcpy(overflow, pCaptureBuffer, overflowSize * _nChannelsIn * sizeof(float));
//
//			}
//
//			assert(_pCaptureDevice->releaseCaptureBuffer(numSamplesAvailable));
//		}
//
//		//We are done with this buffer. Go to next.
//		_bufferIndexCapture = _getNextBufferIndex(_bufferIndexCapture);
//
//		sw.intervalEnd();
//	}
//}

//void CaptureLoop::_wasapiCaptureLoop() {
//	OS::setPriorityHigh();
//	size_t captureLeft, renderAvailable, length, bufferIndex, sampleIndex, channelIndex;
//	UINT32 captureAvailable;
//	float *pCaptureBuffer = nullptr;
//	double *pRenderBuffer;
//	DWORD flags;
//	bool silence = true;
//
//	while (_run) {
//		//Wait until its time to fill the next buffer.
//		_swap.wait();
//
//		sw.intervalStart();
//
//		bufferIndex = _bufferIndexCapture;
//		renderAvailable = _buffers[bufferIndex].getAvailableSize();
//
//		while (renderAvailable > 0) {
//			//Check for samples in capture buffer.
//			assert(_pCaptureDevice->getNextPacketSize(&captureAvailable));
//
//			//No data available
//			if (captureAvailable == 0) {
//				//In silence. Fill buffer with silence and be done.
//				if (silence) {
//					_buffers[bufferIndex].fillWithSilence();
//					break;
//				}
//				//Just waiting for data.
//				else {
//					//Short sleep just to not busy wait all resources.
//					Date::sleepMillis(1);
//					continue;
//				}
//			}
//
//			//Get capture buffer pointer and number of available frames.
//			assert(_pCaptureDevice->getCaptureBuffer(&pCaptureBuffer, &captureAvailable, &flags));
//
//			//Silence flag set. 
//			if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
//				//Was audio before. Reset filter states.
//				if (!silence) {
//					silence = true;
//					_resetFilters();
//				}
//				_buffers[bufferIndex].fillWithSilence();
//			}
//			//Whent from silence to audio playing.
//			else {
//				silence = false;
//				captureLeft = captureAvailable;
//
//				while (captureLeft > 0) {
//					//Available frames to write to buffer.
//					length = min(captureLeft, renderAvailable);
//
//					_buffers[bufferIndex].lock();
//
//					//Copy data from capture buffer to temporary render buffer.
//					pRenderBuffer = _buffers[bufferIndex].getDataWithOffset();
//					for (sampleIndex = 0; sampleIndex < length; ++sampleIndex) {
//						for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
//							//pBuffer[(bufferSize + sampleIndex) * _nChannelsOut + channelIndex] = pCaptureBuffer[sampleIndex * _nChannelsIn + channelIndex];
//							pRenderBuffer[channelIndex] = pCaptureBuffer[channelIndex];
//						}
//						pCaptureBuffer += _nChannelsIn;
//						pRenderBuffer += _nChannelsOut;
//					}
//					_buffers[bufferIndex].releaseData(length);
//
//					_buffers[bufferIndex].unlock();
//
//					captureLeft -= length;
//					renderAvailable -= length;
//
//					if (captureLeft > 0 && renderAvailable == 0) {
//						bufferIndex = _getNextBufferIndex(bufferIndex);
//						renderAvailable = captureLeft;
//#ifdef DEBUG
//						if (renderAvailable > _buffers[bufferIndex].getAvailableSize()) {
//							printf("To small buffer %zd, %zd, %zd\n", bufferIndex, renderAvailable, _buffers[bufferIndex].getAvailableSize());
//						}
//#endif
//					}
//				}
//
//			}
//
//			//Release/flush capture buffer.
//			assert(_pCaptureDevice->releaseCaptureBuffer(captureAvailable));
//		}
//
//
//		//We are done with this buffer. Go to next.
//		_bufferIndexCapture = _getNextBufferIndex(_bufferIndexCapture);
//
//		sw.intervalEnd();
//	}
//
//}

//inline void writeToBuffer(float *pCaptureBuffer, Buffer &buf, int length, int offset) {
//	//Copy data from capture buffer to temporary render buffer.
//	static double* pRenderBuffer;
//	static int _nChannelsIn = 8;
//	static size_t sampleIndex, channelIndex;
//	pRenderBuffer = buf.getDataWithOffset();
//	for (sampleIndex = 0; sampleIndex < length; ++sampleIndex) {
//		for (channelIndex = 0; channelIndex < buf.numChannels(); ++channelIndex) {
//			//pBuffer[(bufferSize + sampleIndex) * _nChannelsOut + channelIndex] = pCaptureBuffer[sampleIndex * _nChannelsIn + channelIndex];
//			pRenderBuffer[channelIndex] = pCaptureBuffer[channelIndex];
//		}
//		pCaptureBuffer += _nChannelsIn;
//		pRenderBuffer += buf.numChannels();
//	}
//	buf.releaseData(length);
//	int a = 2;
//}



//void CaptureLoop::writeToBuffer(float *pCaptureBuffer, long asioBufferIndex, const size_t length, const size_t offset) {
//	static size_t channelIndex, sampleIndex;
//	static int *pRenderBuffer;
//	for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
//		pRenderBuffer = (int*)AsioDevice::pBufferInfos[channelIndex].buffers[asioBufferIndex];
//		for (sampleIndex = 0; sampleIndex < length; ++sampleIndex) {
//			pRenderBuffer[offset + sampleIndex] = (int)(MAX_INT32 * pCaptureBuffer[sampleIndex * _nChannelsIn + channelIndex]);
//		}
//	}
//}