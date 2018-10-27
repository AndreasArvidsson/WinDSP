#include "CaptureLoop.h"
#include "AsioDevice.h"
#include "OS.h"

#define MAX_INT32 2147483648.0

const Config *CaptureLoop::_pConfig;
const std::vector<Input*> *CaptureLoop::_pInputs = nullptr;
const std::vector<Output*> *CaptureLoop::_pOutputs = nullptr;
AudioDevice *CaptureLoop::_pCaptureDevice = nullptr;
AudioDevice *CaptureLoop::_pRenderDevice = nullptr;
double *CaptureLoop::_pClippingChannels = nullptr;
double *CaptureLoop::_pOverflowBuffer = nullptr;
double *CaptureLoop::_renderTmpBuffer = nullptr;
bool *CaptureLoop::_pUsedChannels = nullptr;
size_t CaptureLoop::_nChannelsIn = 0;
size_t CaptureLoop::_nChannelsOut = 0;
size_t CaptureLoop::_renderBlockSize = 0;
UINT32 CaptureLoop::_overflowSize = 0;
bool CaptureLoop::_clippingDetected = false;
bool CaptureLoop::_silent = false;
bool CaptureLoop::_run = false;
std::thread CaptureLoop::_wasapiRenderThread;

void CaptureLoop::init(const Config *pConfig, AudioDevice *pCaptureDevice, AudioDevice *pRenderDevice) {
	_pConfig = pConfig;
	_pInputs = pConfig->getInputs();
	_pOutputs = pConfig->getOutputs();
	_pCaptureDevice = pCaptureDevice;
	_pRenderDevice = pRenderDevice;
	_nChannelsIn = _pInputs->size();
	_nChannelsOut = _pOutputs->size();
	_overflowSize = 0;
	_clippingDetected = _silent = _run = false;

	//The size of all sample frames for all channels with the same sample index/timestamp
	_renderBlockSize = sizeof(double) * _nChannelsOut;

	//Initialize clipping detection
	_pClippingChannels = new double[_nChannelsOut];
	memset(_pClippingChannels, 0, _nChannelsOut * sizeof(double));

	//Initialize conditions
	_pUsedChannels = new bool[_nChannelsIn];
	memset(_pUsedChannels, 0, _nChannelsIn * sizeof(bool));
	Condition::init(_pUsedChannels);
}

void CaptureLoop::destroy() {
	delete[] _pUsedChannels;
	delete[] _pClippingChannels;
	delete[] _renderTmpBuffer;
	delete[] _pOverflowBuffer;
	_pClippingChannels = _renderTmpBuffer = _pOverflowBuffer = nullptr;
	_pUsedChannels = nullptr;
	_pInputs = nullptr;
	_pOutputs = nullptr;
}

void CaptureLoop::run() {
	_run = true;
	_pCaptureDevice->startCaptureService();

	//Asio render device
	if (_pConfig->useAsioRenderer()) {
		//AsioDevice::printInfo();
		ASIOCallbacks callbacks{ 0 };
		callbacks.bufferSwitch = &_bufferSwitch;
		//Used to temporarily store sample(for all out channels) while they are being processed.
		_renderTmpBuffer = new double[_nChannelsOut * AsioDevice::bufferSize];
		//Used to temprary store samples that donest fit in the current render buffer.
		_pOverflowBuffer = new double[_nChannelsOut * AsioDevice::bufferSize];
		//Asio driver automatically starts in new thread
		AsioDevice::startRenderService(&callbacks, (long)_nChannelsOut);
	}
	//Wasapi render device
	else {
		_pRenderDevice->startRenderService();
		//Used to temporarily store sample(for all out channels) while they are being processed.
		_renderTmpBuffer = new double[_nChannelsOut * _pRenderDevice->getBufferSize()];
		//Start rendering in a new thread.
		_wasapiRenderThread = std::thread(_wasapiLoop);
		//_wasapiRenderThread.detach();
	}

	int count = 0;
	for (;;) {
		//Short sleep just to not busy wait all resources.
		Sleep(100);

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
		if (_pConfig->useAsioRenderer()) {
			AsioDevice::stopRenderService();
		}
		else {
			_wasapiRenderThread.join();
		}
	}
}

void CaptureLoop::_wasapiLoop() {
	OS::setPriorityHigh();
	UINT32 i, j, numFramesAvailable;
	float *pRenderBuffer;
	bool silenceToPlayback;
	while (_run) {
		//Wait for device callback
		assertWait(WaitForSingleObjectEx(_pRenderDevice->getEventHandle(), INFINITE, false));

		//Get capture data.
		_getCaptureBuffer(&numFramesAvailable, &silenceToPlayback);

		//No data to render. Just continue and wait for next iteration.
		if (_silent || numFramesAvailable == 0) {
			continue;
		}

		//Was silent before. Needs to flush render buffer before adding new data.
		if (silenceToPlayback) {
			_pRenderDevice->flushRenderBuffer();
		}

		//Must read entire capture buffer at once. Wait until render buffer has enough space available.
		while (numFramesAvailable > _pRenderDevice->getBufferFrameCountAvailable()) {
			//Short sleep just to not busy wait all resources.
			Sleep(1);
		}

		//Get render buffer
		assert(_pRenderDevice->getRenderBuffer(&pRenderBuffer, numFramesAvailable));

		//Copy frames from tmp render buffer to actual render buffer.
		for (i = 0; i < numFramesAvailable; ++i) {
			for (j = 0; j < _nChannelsOut; ++j) {
				pRenderBuffer[i * _nChannelsOut + j] = (float)_renderTmpBuffer[i * _nChannelsOut + j];
			}
		}

		//Release/flush render buffer.
		assert(_pRenderDevice->releaseRenderBuffer(numFramesAvailable));
	}
}

void CaptureLoop::_bufferSwitch(const long bufferIndex, const ASIOBool) {
	static UINT32 numFramesAvailable, length, framesLeft;
	framesLeft = AsioDevice::bufferSize;

	//Have data leftover from last call. Process these first.
	if (_overflowSize > 0) {
		length = min(_overflowSize, framesLeft);
		_writeToBuffer(_pOverflowBuffer, length, bufferIndex);
		_overflowSize -= length;
		framesLeft -= length;
	}

	//Must fill entire render buffer before returning from func.
	while (framesLeft > 0) {
		//Get capture data.
		_getCaptureBuffer(&numFramesAvailable);

		//Silence. Just write silence and return to swap buffer.
		if (_silent) {
			AsioDevice::renderSilence(bufferIndex);
			return;
		}

		//No data in capture device right now. Just continue and check again.
		if (numFramesAvailable == 0) {
			//Short sleep just to not busy wait all resources.
			Sleep(1);
			continue;
		}

		length = min(numFramesAvailable, framesLeft);

		//Write samples to render buffer.
		_writeToBuffer(_renderTmpBuffer, length, bufferIndex, AsioDevice::bufferSize - framesLeft);

		framesLeft -= length;
	}

	//Data left in capture buffer. Store in overflow.
	if (numFramesAvailable > length) {
		_overflowSize = numFramesAvailable - length;
		memcpy(_pOverflowBuffer, _renderTmpBuffer + length * _nChannelsOut, _overflowSize * _nChannelsOut * sizeof(double));
	}

	//If available this can reduce letency.
	if (AsioDevice::outputReady) {
		assertAsio(ASIOOutputReady());
	}
}

void CaptureLoop::_writeToBuffer(const double* const pSource, const UINT32 length, const int bufferIndex, const UINT32 offset) {
	for (size_t channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
		int *pRenderBuffer = pRenderBuffer = (int*)AsioDevice::pBufferInfos[channelIndex].buffers[bufferIndex];
		for (size_t sampleIndex = 0; sampleIndex < length; ++sampleIndex) {
			pRenderBuffer[offset + sampleIndex] = (int)(MAX_INT32 * pSource[sampleIndex * _nChannelsOut + channelIndex]);
		}
	}
}

void CaptureLoop::_getCaptureBuffer(UINT32 * const pNumFramesAvailable, bool * const pSilenceToPlayback) {
	//Check for samples in capture buffer.
	assert(_pCaptureDevice->getNextPacketSize(pNumFramesAvailable));

	//No data in capture device. Just return.
	if (*pNumFramesAvailable == 0) {
		return;
	}

	//Get capture buffer pointer and number of available frames.
	static float *pCaptureBuffer;
	static DWORD flags;
	assert(_pCaptureDevice->getCaptureBuffer(&pCaptureBuffer, pNumFramesAvailable, &flags));

	//Silence. Do NOT send anything to render buffer.
	if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
		//Was audio before. Reset filter states.
		if (!_silent) {
			_silent = true;
			_resetFilters();
		}
	}
	//Audio playing. Send to render buffer.
	else {
		if (pSilenceToPlayback) {
			//Is true if it was silent before
			*pSilenceToPlayback = _silent;
		}
		_silent = false;

		//Parse input data, apply routes and filters and fill temporary render buffer.
		_fillRenderTmpBuffer(pCaptureBuffer, *pNumFramesAvailable);
	}

	//Release/flush capture buffer.
	assert(_pCaptureDevice->releaseCaptureBuffer(*pNumFramesAvailable));
}

void CaptureLoop::_fillRenderTmpBuffer(float* captureBuffer, const UINT32 length) {
	//Set default value to 0 so we can add/mix values to it later
	memset(_renderTmpBuffer, 0, _renderBlockSize * length);

	//Iterate all capture frames
	static double* renderBuffer;;
	renderBuffer = _renderTmpBuffer;
	static UINT32 i, j;
	for (i = 0; i < length; ++i) {
		//Iterate inputs
		for (j = 0; j < _nChannelsIn; ++j) {
			//Identify which channels are playing.
			if (captureBuffer[j] != 0.0f) {
				_pUsedChannels[j] = true;
			}
			//Route sample to outputs
			(*_pInputs)[j]->route(captureBuffer[j], renderBuffer);
		}

		//Iterate outputs
		for (j = 0; j < _nChannelsOut; ++j) {
			//Apply output forks and filters
			renderBuffer[j] = (*_pOutputs)[j]->process(renderBuffer[j]);

			//Check for clipping
			if (abs(renderBuffer[j]) > 1.0) {
				_clippingDetected = true;
				_pClippingChannels[j] = max(_pClippingChannels[j], abs(renderBuffer[j]));
			}
		}

		//Move buffers to next sample
		captureBuffer += _nChannelsIn;
		renderBuffer += _nChannelsOut;
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
	for (size_t i = 0; i < _nChannelsOut; ++i) {
		if (_pClippingChannels[i] != 0) {
			printf("WARNING: Output(%s) - Clipping detected: +%0.2f dBFS\n", _pConfig->getChannelName(i).c_str(), Convert::levelToDb(_pClippingChannels[i]));
			_pClippingChannels[i] = 0;
		}
	}
}

void CaptureLoop::_updateConditionalRouting() {
	//Update conditional routing.
	for (const Input *pInut : *_pInputs) {
		pInut->evalConditions();
	}
	//Set all to false so we can check anew until next time this func runs.
	for (size_t i = 0; i < _nChannelsIn; ++i) {
		_pUsedChannels[i] = false;
	}
}

//For debug purposes only.
void CaptureLoop::_printUsedChannels() {
	for (size_t i = 0; i < _nChannelsIn; ++i) {
		printf("%s %d\n", _pConfig->getChannelName(i).c_str(), _pUsedChannels[i]);
	}
	printf("\n");
}