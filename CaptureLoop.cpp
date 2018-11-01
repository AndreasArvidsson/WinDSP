#include "CaptureLoop.h"
#include "AsioDevice.h"

#ifdef DEBUG
#define swStart() (void)0
#define swEnd() (void)0
#else
//#include "Stopwatch.h"
//Stopwatch sw("Render", 500);
//#define swStart() sw.intervalStart()
//#define swEnd() sw.intervalEnd()
#define swStart() (void)0
#define swEnd() (void)0
#endif

#define MAX_INT32 2147483648.0

const Config *CaptureLoop::_pConfig;
const std::vector<Input*> *CaptureLoop::_pInputs = nullptr;
const std::vector<Output*> *CaptureLoop::_pOutputs = nullptr;
AudioDevice *CaptureLoop::_pCaptureDevice = nullptr;
AudioDevice *CaptureLoop::_pRenderDevice = nullptr;
size_t CaptureLoop::_nChannelsIn = 0;
size_t CaptureLoop::_nChannelsOut = 0;
UINT32 CaptureLoop::_renderBufferCapacity = 0;
std::thread CaptureLoop::_wasapiRenderThread;
std::atomic<bool> CaptureLoop::_run = false;
std::atomic<bool> CaptureLoop::_throwError = false;
bool CaptureLoop::_silence = false;
double *CaptureLoop::_pProcessBuffer = nullptr;
size_t CaptureLoop::_overflowSize = 0;
float *CaptureLoop::_pOverflowBuffer = nullptr;
bool *CaptureLoop::_pUsedChannels;
Error CaptureLoop::_error;

void CaptureLoop::init(const Config *pConfig, AudioDevice *pCaptureDevice, AudioDevice *pRenderDevice) {
	_pConfig = pConfig;
	_pInputs = pConfig->getInputs();
	_pOutputs = pConfig->getOutputs();
	_pCaptureDevice = pCaptureDevice;
	_pRenderDevice = pRenderDevice;
	_nChannelsIn = _pInputs->size();
	_nChannelsOut = _pOutputs->size();
	_renderBufferCapacity = _pCaptureDevice->getEngineBufferSize();
	_pProcessBuffer = new double[_renderBufferCapacity * _nChannelsOut];
	_pOverflowBuffer = new float[_renderBufferCapacity * _nChannelsIn];

	//Initialize conditions
	_pUsedChannels = new bool[_nChannelsIn];
	memset(_pUsedChannels, 0, _nChannelsIn * sizeof(bool));
	Condition::init(_pUsedChannels);
}

void CaptureLoop::destroy() {
	delete[] _pProcessBuffer;
	delete[] _pOverflowBuffer;
	delete[] _pUsedChannels;
	_pProcessBuffer = nullptr;
	_pOverflowBuffer = nullptr;
	_pUsedChannels = nullptr;
	_pInputs = nullptr;
	_pOutputs = nullptr;
}

void CaptureLoop::run() {
	_run = _silence = true;
	_throwError = false;
	_overflowSize = 0;

	//Start wasapi capture device.
	_pCaptureDevice->startService();

	//Asio render device.
	if (_pConfig->useAsioRenderer()) {
		ASIOCallbacks callbacks{ 0 };
		callbacks.asioMessage = &_asioMessage;
		callbacks.bufferSwitch = &_asioRenderCallback;
		//Asio driver automatically starts in new thread
		AsioDevice::startRenderService(&callbacks, (long)_renderBufferCapacity, (long)_nChannelsOut);
	}
	//Wasapi render device.
	else {
		_pRenderDevice->startService();
		//Start rendering in a new thread.
		_wasapiRenderThread = std::thread(_wasapiRenderLoop);
	}

	size_t count = 0;
	for (;;) {
		//Asio renderer operates in its on thread context and cant directly throw exceptions.
		if (_throwError) {
			throw _error;
		}

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

void CaptureLoop::_asioRenderCallback(const long asioBufferIndex, const ASIOBool) {
	swStart();

	//Process data in capture buffer and fill the process buffer.
	_fillProcessBuffer(_renderBufferCapacity);

	//Copy data from process buffer to render buffer.
	for (size_t channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
		int* pRenderBuffer = AsioDevice::getBuffer(channelIndex, asioBufferIndex);
		for (size_t sampleIndex = 0; sampleIndex < _renderBufferCapacity; ++sampleIndex) {
			//Apply output forks and filters and then convert from float to int.
			pRenderBuffer[sampleIndex] = (int)(MAX_INT32 * (*_pOutputs)[channelIndex]->process(_pProcessBuffer[sampleIndex * _nChannelsOut + channelIndex]));
		}
	}

	//If available this can reduce letency.
	AsioDevice::outputReady();

	swEnd();
}

void CaptureLoop::_wasapiRenderLoop() {
	float *pRenderBuffer;
	UINT32 captureAvailable;
	size_t sampleIndex, channelIndex;
	while (_run) {
		//Check for samples in capture buffer.
		assert(_pCaptureDevice->getNextPacketSize(&captureAvailable));

		while (captureAvailable != 0) {
			//Process data in capture buffer and fill the process buffer.
			_fillProcessBuffer(captureAvailable);

			//Must read entire capture buffer at once. Wait until render buffer has enough space available.
			while (captureAvailable > _pRenderDevice->getBufferFrameCountAvailable()) {
				//Short sleep just to not busy wait all resources.
				Date::sleepMillis(1);
			}

			//Get render buffer
			assert(_pRenderDevice->getRenderBuffer(&pRenderBuffer, captureAvailable));

			//Copy data from process buffer to render buffer.
			for (sampleIndex = 0; sampleIndex < captureAvailable; ++sampleIndex) {
				for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
					//Apply output forks and filters.
					pRenderBuffer[sampleIndex * _nChannelsOut + channelIndex] = (float)(*_pOutputs)[channelIndex]->process(_pProcessBuffer[sampleIndex * _nChannelsOut + channelIndex]);
				}
			}

			//Release/flush render buffer.
			assert(_pRenderDevice->releaseRenderBuffer(captureAvailable));

			//Check for even more samples in capture buffer.
			assert(_pCaptureDevice->getNextPacketSize(&captureAvailable));
		}

		//Short sleep just to not busy wait all resources.
		Date::sleepMillis(1);
	}
}

void CaptureLoop::_fillProcessBuffer(size_t renderLeft) {	UINT32 captureAvailable;
	size_t length;
	float *pCaptureBuffer;
	DWORD flags;

	//Overflow from last pass. Use these first.
	if (_overflowSize) {
		length = min(renderLeft, _overflowSize);
		_processCaptureBuffer(_pOverflowBuffer, length);
		_overflowSize = 0;
		renderLeft -= length;
	}

	while (renderLeft > 0) {
		//Check for samples in capture buffer.
		assert(_pCaptureDevice->getNextPacketSize(&captureAvailable));

		//No data available
		if (captureAvailable == 0) {
			//In silence 'mode'. Fill buffer with silence and be done.
			if (_silence) {
				_fillProcessBufferWithSilence();
				return;
			}
			//Just waiting for data during playback.
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
			if (!_silence) {
				_silence = true;
				_resetFilters();
			}
			_fillProcessBufferWithSilence();
			renderLeft = 0;
		}
		else {
			_silence = false;

			//Available frames to write to buffer.
			length = min(renderLeft, captureAvailable);

			//Copy data from capture buffer to temporary render buffer.
			_processCaptureBuffer(pCaptureBuffer, length, _renderBufferCapacity - renderLeft);

			if (captureAvailable > length) {
				_overflowSize = captureAvailable - length;
				memcpy(_pOverflowBuffer, pCaptureBuffer + length * _nChannelsIn, _overflowSize * _nChannelsIn * sizeof(float));
			}

			renderLeft -= length;
		}

		//Release/flush capture buffer.
		assert(_pCaptureDevice->releaseCaptureBuffer(captureAvailable));
	}
}

void CaptureLoop::_processCaptureBuffer(const float * pCaptureBuffer, const size_t length, const size_t offset) {
	double *pProcessBuffer = _pProcessBuffer + offset * _nChannelsOut;

	//Set default value to 0 so we can add/mix values to it later
	memset(pProcessBuffer, 0, length * _nChannelsOut * sizeof(double));

	//Iterate inputs and route samples from input to process buffer.
	for (size_t sampleIndex = 0; sampleIndex < length; ++sampleIndex) {
		for (size_t channelIndex = 0; channelIndex < _nChannelsIn; ++channelIndex) {
			(*_pInputs)[channelIndex]->route(pCaptureBuffer[sampleIndex * _nChannelsIn + channelIndex], pProcessBuffer);
		}
		pProcessBuffer += _nChannelsOut;
	}
}

void CaptureLoop::_fillProcessBufferWithSilence() {
	memset(_pProcessBuffer, 0, _renderBufferCapacity * _nChannelsOut * sizeof(double));
}

long CaptureLoop::_asioMessage(const long selector, const long value, void* const message, double* const opt) {
	switch (selector) {
	case kAsioSelectorSupported:
		switch (value) {
		case kAsioResetRequest:
		case kAsioResyncRequest:
			return 1L;
		}
		break;
		//Ask the host application for its ASIO implementation. Host ASIO implementation version, 2 or higher 
	case kAsioEngineVersion:
		return 2L;
		//Requests a driver reset. Return value is always 1L.
	case kAsioResetRequest:
		_error = Error("ASIO hardware is not available or has been reset.");
		_throwError = true;
		return 1L;
		//The driver went out of sync, such that the timestamp is no longer valid.This is a request to re -start the engine and slave devices(sequencer). 1L if request is accepted or 0 otherwise.
	case kAsioResyncRequest:
		_error = Error("ASIO driver went out of sync.");
		_throwError = true;
		return 1L;
	}
	return 0L;
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
	for (Output *pOutput : *_pOutputs) {
		if (pOutput->clipping != 0.0) {
			printf("WARNING: Output(%s) - Clipping detected: +%0.2f dBFS\n", pOutput->getName().c_str(), Convert::levelToDb(pOutput->clipping));
			pOutput->clipping = 0.0;
		}
	}
}

void CaptureLoop::_updateConditionalRouting() {
	//Get current is playing status per channel.
	for (size_t i = 0; i < _nChannelsIn; ++i) {
		_pUsedChannels[i] = (*_pInputs)[i]->resetIsPlaying();
	}

	//Update conditional routing.
	for (const Input *pInut : *_pInputs) {
		pInut->evalConditions();
	}
}

//For debug purposes only.
void CaptureLoop::_printUsedChannels() {
	for (size_t i = 0; i < _nChannelsIn; ++i) {
		printf("%s %d\n", (*_pInputs)[i]->getName().c_str(), _pUsedChannels[i]);
	}
	printf("\n");
}