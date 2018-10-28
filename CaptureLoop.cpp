#include "CaptureLoop.h"
#include "AsioDevice.h"
#include "OS.h"

#define MAX_INT32 2147483648.0

const Config *CaptureLoop::_pConfig;
const std::vector<Input*> *CaptureLoop::_pInputs = nullptr;
const std::vector<Output*> *CaptureLoop::_pOutputs = nullptr;
AudioDevice *CaptureLoop::_pCaptureDevice = nullptr;
AudioDevice *CaptureLoop::_pRenderDevice = nullptr;
size_t CaptureLoop::_nChannelsIn = 0;
size_t CaptureLoop::_nChannelsOut = 0;
size_t CaptureLoop::_renderBlockSize = 0;
size_t CaptureLoop::_renderBufferCapacity = 0;
std::thread CaptureLoop::_wasapiCaptureThread;
std::thread CaptureLoop::_wasapiRenderThread;
Buffer CaptureLoop::_buffers[NUM_BUFFERS];
bool CaptureLoop::_run = false;
std::atomic<bool> CaptureLoop::_silent = false;
std::atomic<size_t>  CaptureLoop::_bufferIndexCapture = 0;
std::atomic<size_t> CaptureLoop::_bufferIndexRender = 0;
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
	_bufferIndexRender = 0;
	_bufferIndexCapture = 1;
	_resetBuffers();
	_run = _silent = true;

	//_pCaptureDevice->printInfo();

	//Asio render device
	if (_pConfig->useAsioRenderer()) {
		//AsioDevice::printInfo();
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

void CaptureLoop::_wasapiCaptureLoop() {
	OS::setPriorityHigh();
	size_t captureLeft, renderAvailable, length, bufferIndex, sampleIndex, channelIndex;
	UINT32 captureAvailable;
	float *pCaptureBuffer = nullptr;
	double *pRenderBuffer;
	DWORD flags;
	bool silence = true;
	bool signalPlayback = false;
	while (_run) {
		//Wait until its time to fill the next buffer.
		_waitSwap();

		bufferIndex = _bufferIndexCapture;
		renderAvailable = _buffers[bufferIndex].getAvailableSize();

		while (renderAvailable > 0) {
			//Check for samples in capture buffer.
			assert(_pCaptureDevice->getNextPacketSize(&captureAvailable));

			//No data and no silence. 
			if (captureAvailable == 0) {
				//Short sleep just to not busy wait all resources.
				Date::sleepMillis(1);
				continue;
			}

			//Get capture buffer pointer and number of available frames.
			assert(_pCaptureDevice->getCaptureBuffer(&pCaptureBuffer, &captureAvailable, &flags));

			//Silence flag set. 
			if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
				//Was audio before. Reset filter states.
				if (!silence) {
					silence = true;
					_silent = true;
					_resetFilters();
				}
			}
			//Whent from silence to audio playing.
			else if (silence) {
				_resetBuffers();
				renderAvailable = _buffers[bufferIndex].getAvailableSize();
				silence = false;
				signalPlayback = true;
			}

			if (silence) {
				assert(_pCaptureDevice->releaseCaptureBuffer(captureAvailable));
				//Short sleep just to not busy wait all resources.
				Date::sleepMillis(1);
				continue;
			}

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

				if (captureLeft > 0 && renderAvailable == 0) {
					bufferIndex = _getNextBufferIndex(bufferIndex);
					renderAvailable = captureLeft;
#ifdef DEBUG
					if (renderAvailable > _buffers[bufferIndex].getAvailableSize()) {
						printf("To small buffer %zd, %zd, %zd\n", bufferIndex, renderAvailable, _buffers[bufferIndex].getAvailableSize());
					}
					if (_bufferIndexRender == bufferIndex) {
						printf("Same index %zd\n", bufferIndex);
					}
#endif
				}

			}

			//Release/flush capture buffer.
			assert(_pCaptureDevice->releaseCaptureBuffer(captureAvailable));
		}

		//Have to do this after buffer is filled or else the playbakc would start on the previous buffer with bad data.
		if (signalPlayback) {
			signalPlayback = false;
			_silent = false;
		}

	}
}

void CaptureLoop::_bufferSwitch(const long asioBufferIndex, const ASIOBool) {
	static size_t channelIndex, sampleIndex;
	static int *pRenderBuffer;
	static double *pCaptureBuffer;

	//Silence. Just write 0 to render buffer.
	if (_silent) {
		AsioDevice::renderSilence(asioBufferIndex);
	}
	//Audio playing. copy from buffer.
	else {
		//Copy data from temporary render buffer to the real deal.
		pCaptureBuffer = _buffers[_bufferIndexRender].getData();
		for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
			pRenderBuffer = (int*)AsioDevice::pBufferInfos[channelIndex].buffers[asioBufferIndex];
			for (sampleIndex = 0; sampleIndex < _renderBufferCapacity; ++sampleIndex) {
				pRenderBuffer[sampleIndex] = (int)(MAX_INT32 * pCaptureBuffer[sampleIndex * _nChannelsOut + channelIndex]);
			}
		}
		_buffers[_bufferIndexRender].releaseData();

		//Signal that we are done with this buffer.
		_advanceBufferIndex();
	}

	//Notify capture thread that this buffer is done.
	_notifySwap();

	//If available this can reduce letency.
	if (AsioDevice::outputReady) {
		assertAsio(ASIOOutputReady());
	}
}

void CaptureLoop::_wasapiRenderLoop() {
	OS::setPriorityHigh();
	size_t channelIndex, sampleIndex, bufferIndex;
	UINT32 numFramesAvailable;
	float *pRenderBuffer;
	double *pCaptureBuffer;
	while (_run) {
		//Wait for device callback
		assertWait(WaitForSingleObjectEx(_pRenderDevice->getEventHandle(), INFINITE, false));

		//Get number of available framesd in the render buffer.
		numFramesAvailable = _pRenderDevice->getBufferFrameCountAvailable();

		//No space to render or nothing playing. Just ignore.
		if (numFramesAvailable == 0 || _silent) {
			//No need to write silence to wasapi endpoint. Just leave it alone.
		}
		//Audio playing. copy from buffer.
		else {
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

			//Signal that we are done with this buffer.
			_advanceBufferIndex();

			//Release/flush render buffer.
			assert(_pRenderDevice->releaseRenderBuffer(_renderBufferCapacity));
		}

		//Notify capture thread that this buffer is done.
		_notifySwap();
	}
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