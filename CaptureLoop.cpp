#include "CaptureLoop.h"

CaptureLoop::CaptureLoop(Config *pConfig, const AudioDevice *pCaptureDevice, const AudioDevice *pRenderDevice, const std::vector<Input*> &inputs, const std::vector<Output*> &outputs) {
	_pConfig = pConfig;
	_pCaptureDevice = pCaptureDevice;
	_pRenderDevice = pRenderDevice;
	_inputs = inputs;
	_outputs = outputs;
	_nChannelsIn = inputs.size();
	_nChannelsOut = outputs.size();

	//Used to temporarily store sample(for all out channels) while they are being processed.
	_renderBlockBuffer = new double[_nChannelsOut];

	//Initialize clipping detection
	_pClippingChannels = new float[_nChannelsOut];
	memset(_pClippingChannels, 0, _nChannelsOut * sizeof(float));

	//Initialize conditions
	_pUsedChannels = new bool[_nChannelsIn];
	memset(_pUsedChannels, 0, _nChannelsIn * sizeof(bool));
	Condition::init(_pUsedChannels);
}

CaptureLoop::~CaptureLoop() {
	delete[] _pUsedChannels;
	delete[] _pClippingChannels;
	delete[] _renderBlockBuffer;
}

void CaptureLoop::capture() {
	UINT32 i, j, numFramesAvailable;
	float *pCaptureBuffer, *pRenderBuffer;
	DWORD flags;
	//The size of all sample frames for all channels with the same sample index/timestamp
	const size_t renderBlockSize = sizeof(double) * _nChannelsOut;
	time_t lastCritical = 0;;
	time_t lastNotCritical = 0;
	bool clippingDetected = false;
	bool silent = true;

	//Run infinite capture loop
	while (true) {
		//Check for samples in capture buffer
		assert(_pCaptureDevice->getNextPacketSize(&numFramesAvailable));

		while (numFramesAvailable != 0) {
			//Get capture buffer pointer and number of available frames.
			assert(_pCaptureDevice->getCaptureBuffer(&pCaptureBuffer, &numFramesAvailable, &flags));

			//Silence. Do NOT send anything to render buffer.
			if (flags == AUDCLNT_BUFFERFLAGS_SILENT) {
				//Was audio before. Reset filter states.
				if (!silent) {
					silent = true;
					resetFilters();
				}
			}
			//Audio playing. Send to render buffer.
			else {
				//Was silent before. Needs to flush render buffer before adding new data.
				if (silent) {
					silent = false;
					_pRenderDevice->flushRenderBuffer();
				}

				//Must read entire capture buffer at once. Wait until render buffer has enough space available.
				while (numFramesAvailable > _pRenderDevice->getBufferFrameCountAvailable()) {
					//Short sleep just to not busy wait all resources.
					Date::sleepMillis(1);
				}

				//Get render buffer
				assert(_pRenderDevice->getRenderBuffer(&pRenderBuffer, numFramesAvailable));

				//Iterate all capture frames
				for (i = 0; i < numFramesAvailable; ++i) {
					//Set default value to 0 so we can add/mix values to it later
					memset(_renderBlockBuffer, 0, renderBlockSize);

					//Iterate inputs
					for (j = 0; j < _nChannelsIn; ++j) {
						//Identify which channels are playing.
						if (pCaptureBuffer[j] != 0) {
							_pUsedChannels[j] = true;
						}
						//Route sample to outputs
						_inputs[j]->route(pCaptureBuffer[j], _renderBlockBuffer);
					}

					//Iterate outputs
					for (j = 0; j < _nChannelsOut; ++j) {
						//Apply output forks and filters
						pRenderBuffer[j] = (float)_outputs[j]->process(_renderBlockBuffer[j]);

						//Check for clipping
						if (abs(pRenderBuffer[j]) > 1.0) {
							clippingDetected = true;
							_pClippingChannels[j] = max(_pClippingChannels[j], abs(pRenderBuffer[j]));
							//Set to max value. Avoid wraparound if converting to int.
							pRenderBuffer[j] = 1.0;
						}
					}

					//Move buffers to next sample
					pCaptureBuffer += _nChannelsIn;
					pRenderBuffer += _nChannelsOut;
				}

				//Release/flush render buffer.
				assert(_pRenderDevice->releaseRenderBuffer(numFramesAvailable));
			}

			//Release/flush capture buffer.
			assert(_pCaptureDevice->releaseCaptureBuffer(numFramesAvailable));

			//Run always. Check time critical changes.
			if (Date::getCurrentTimeMillis() - lastCritical > 1000) {
				lastCritical = Date::getCurrentTimeMillis();
				//Check if config file has changed
				checkConfig();
			}

			//Check for samples in capture buffer
			assert(_pCaptureDevice->getNextPacketSize(&numFramesAvailable));
		}

		//No input data. Take this time to check NOT time critical changes.
		if (Date::getCurrentTimeMillis() - lastNotCritical > 1000) {
			lastNotCritical = lastCritical = Date::getCurrentTimeMillis();
			//Check if config file has changed. Needed here so that config can change when audio is not playing.
			checkConfig();

			//Update conditional routing if used.
			if (_pConfig->useConditionalRouting()) {
				updateConditionalRouting();
			}
			//Check for clipping output channels
			if (clippingDetected) {
				clippingDetected = false;
				checkClippingChannels();
			}
		}
		else {
			//Short sleep just to not busy wait all resources.
			Date::sleepMillis(1);
		}
	}
}

void CaptureLoop::checkConfig() {
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

void CaptureLoop::updateConditionalRouting() {
	//Update conditional routing.
	for (const Input *pInut : _inputs) {
		pInut->evalConditions();
	}
	//Set all to false so we can check anew until next time this func runs.
	for (size_t i = 0; i < _nChannelsIn; ++i) {
		_pUsedChannels[i] = false;
	}
}

void CaptureLoop::checkClippingChannels() {
	for (size_t i = 0; i < _nChannelsOut; ++i) {
		if (_pClippingChannels[i] != 0) {
			printf("WARNING: Output(%s) - Clipping detected: +%0.2f dBFS\n", _pConfig->getChannelName(i).c_str(), Convert::levelToDb(_pClippingChannels[i]));
			_pClippingChannels[i] = 0;
		}
	}
}

void CaptureLoop::resetFilters() {
	//Reset i/o filter states.
	for (Input *p : _inputs) {
		p->reset();
	}
	for (Output *p : _outputs) {
		p->reset();
	}
}

//For debug purposes only.
void CaptureLoop::printUsedChannels() const {
	for (size_t i = 0; i < _nChannelsIn; ++i) {
		printf("%s %d\n", _pConfig->getChannelName(i).c_str(), _pUsedChannels[i]);
	}
	printf("\n");
}