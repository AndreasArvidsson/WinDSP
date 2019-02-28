#include "CaptureLoop.h"
#include "WinDSPLog.h"
#include "Keyboard.h"
#include "Date.h"
#include "Convert.h"
#include "ConfigChangedException.h"
#include "Config.h"
#include "TrayIcon.h"

//#define PERFORMANCE_LOG

#ifdef PERFORMANCE_LOG
#include "Stopwatch.h"
Stopwatch sw("Render", 1000);
#define swStart() sw.intervalStart()
#define swEnd() sw.intervalEnd()
#else
#define swStart() (void)0
#define swEnd() (void)0
#endif

const Config *CaptureLoop::_pConfig;
const std::vector<Input*> *CaptureLoop::_pInputs = nullptr;
const std::vector<Output*> *CaptureLoop::_pOutputs = nullptr;
AudioDevice *CaptureLoop::_pCaptureDevice = nullptr;
AudioDevice *CaptureLoop::_pRenderDevice = nullptr;
size_t CaptureLoop::_nChannelsIn = 0;
size_t CaptureLoop::_nChannelsOut = 0;
std::thread CaptureLoop::_wasapiCaptureThread;
std::atomic<bool> CaptureLoop::_run = false;
bool *CaptureLoop::_pUsedChannels;

void CaptureLoop::init(const Config *pConfig, AudioDevice *pCaptureDevice, AudioDevice *pRenderDevice) {
	_pConfig = pConfig;
	_pInputs = pConfig->getInputs();
	_pOutputs = pConfig->getOutputs();
	_pCaptureDevice = pCaptureDevice;
	_pRenderDevice = pRenderDevice;
	_nChannelsIn = _pInputs->size();
	_nChannelsOut = _pOutputs->size();

	//Initialize conditions
	_pUsedChannels = new bool[_nChannelsIn];
	memset(_pUsedChannels, 0, _nChannelsIn * sizeof(bool));
	Condition::init(_pUsedChannels);
}

void CaptureLoop::destroy() {
	delete[] _pUsedChannels;
	_pUsedChannels = nullptr;
	_pInputs = nullptr;
	_pOutputs = nullptr;
}

void CaptureLoop::run() {
	_run = true;

	//Start wasapi capture device.
	_pCaptureDevice->startService();
	//Start wasapi render device.
	_pRenderDevice->startService();

	//Start capturing in a new thread.
	_wasapiCaptureThread = std::thread(_wasapiCaptureLoop);

	size_t count = 0;
	while (_run) {
		//Short sleep just to not busy wait all resources.
		Date::sleepMillis(100);

		//Check if config file has changed.
		_checkConfig();

		//Check if tray icon is clicked.
		TrayIcon::handleQueue();

		//Dont do as often. Every 5second or so.
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

void CaptureLoop::_wasapiCaptureLoop() {
	//Used to temporarily store sample(for all out channels) while they are being processed.
	double renderBlockBuffer[16];
	//The size of all sample frames for all channels with the same sample index/timestamp
	const size_t renderBlockSize = sizeof(double) * _nChannelsOut;
	UINT32 channelIndex, sampleIndex, captureAvailable;
	DWORD flags;
	float *pCaptureBuffer, *pRenderBuffer;
	bool silent = true;
	bool first = true;

	while (_run) {
		//Check for samples in capture buffer.
		assert(_pCaptureDevice->getNextPacketSize(&captureAvailable));

		while (captureAvailable != 0) {
			//Get capture buffer pointer and number of available frames.
			assert(_pCaptureDevice->getCaptureBuffer(&pCaptureBuffer, &captureAvailable, &flags));

			if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
				//Was playing audio before. Reset filter states.
				if (!silent) {
					silent = true;
					_resetFilters();
				}
				assert(_pCaptureDevice->releaseCaptureBuffer(captureAvailable));
				continue;
			}
		
			//Was silent before.
			if (silent) {
				silent = false;
				//First frames in capture buffer are bad for some strange reason. Part of the Wasapi standard.
				//Need to flush buffer of bad data or we can get glitches.
				if (first) {
					first = false;
					assert(_pCaptureDevice->releaseCaptureBuffer(captureAvailable));
					_pCaptureDevice->flushCaptureBuffer();
					break;
				}
			}

			if (flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) {
				LOG_WARN("%s: AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY: %d\n", Date::getLocalDateTimeString().c_str(), captureAvailable);
			}

			//Must read entire capture buffer at once. Wait until render buffer has enough space available.
			while (captureAvailable > _pRenderDevice->getBufferFrameCountAvailable()) {
				//Short sleep just to not busy wait all resources.
				Date::sleepMicros(1);
			}

			//Get render buffer
			assert(_pRenderDevice->getRenderBuffer(&pRenderBuffer, captureAvailable));
		
			swStart();

			//Iterate all capture frames
			for (sampleIndex = 0; sampleIndex < captureAvailable; ++sampleIndex) {
				//Set buffer default value to 0 so we can add/mix values to it later
				memset(renderBlockBuffer, 0, renderBlockSize);

				//Iterate inputs and route samples to outputs
				for (channelIndex = 0; channelIndex < _nChannelsIn; ++channelIndex) {
					(*_pInputs)[channelIndex]->route(pCaptureBuffer[channelIndex], renderBlockBuffer);
				}

				//Iterate outputs and apply output forks and filters
				for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
					pRenderBuffer[channelIndex] = (float)(*_pOutputs)[channelIndex]->process(renderBlockBuffer[channelIndex]);
				}

				//Move buffers to next sample
				pCaptureBuffer += _nChannelsIn;
				pRenderBuffer += _nChannelsOut;
			}

			swEnd();

			//Release render buffer.
			assert(_pRenderDevice->releaseRenderBuffer(captureAvailable));

			//Release capture buffer.
			assert(_pCaptureDevice->releaseCaptureBuffer(captureAvailable));

			//Check for more samples in capture buffer.
			assert(_pCaptureDevice->getNextPacketSize(&captureAvailable));
		}

		//No available samples. Short sleep just to not busy wait all resources.
		Date::sleepMicros(1);
	}
}

void CaptureLoop::stop() {
	if (_run) {
		_run = false;
		//Stop and wait for wasapi thread to finish.
		_wasapiCaptureThread.join();
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
			LOG_WARN("WARNING: Output(%s) - Clipping detected: +%0.2f dBFS\n", pOutput->getName().c_str(), Convert::levelToDb(pOutput->clipping));
			pOutput->clipping = 0.0;
		}
	}
}

void CaptureLoop::_updateConditionalRouting() {
	if (_pConfig->useConditionalRouting()) {
		//Get current is playing status per channel.
		for (size_t i = 0; i < _nChannelsIn; ++i) {
			_pUsedChannels[i] = (*_pInputs)[i]->resetIsPlaying();
		}
		//Update conditional routing.
		for (const Input *pInut : *_pInputs) {
			pInut->evalConditions();
		}
	}
}

//For debug purposes only.
void CaptureLoop::_printUsedChannels() {
	for (size_t i = 0; i < _nChannelsIn; ++i) {
		LOG_INFO("%s %d\n", (*_pInputs)[i]->getName().c_str(), _pUsedChannels[i]);
	}
	LOG_NL();
}