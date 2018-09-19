#include "CaptureLoop.h"

CaptureLoop::CaptureLoop(Config *pConfig, const AudioDevice *pCaptureDevice, const AudioDevice *pRenderDevice, const std::vector<Input*> &inputs, const std::vector<Output*> &outputs) {
	_pConfig = pConfig;
	_pCaptureDevice = pCaptureDevice;
	_pRenderDevice = pRenderDevice;
	_inputs = inputs;
	_outputs = outputs;
	_nChannelsIn = inputs.size();
	_nChannelsOut = outputs.size();

	//Initialize clipping detection
	_pClippingChannels = new float[_nChannelsOut];
	memset(_pClippingChannels, 0, _nChannelsOut * sizeof(float));

	//Initialize conditions
	_pUsedChannels = new time_t[_nChannelsIn];
	memset(_pUsedChannels, 0, _nChannelsIn * sizeof(time_t));
	Condition::init(_pUsedChannels);
}

CaptureLoop::~CaptureLoop() {
	delete[] _pUsedChannels;
	delete[] _pClippingChannels;
}

void CaptureLoop::capture() {
	UINT32 numFramesAvailable, i, j;
	time_t now;
	time_t lastConfig = Date::getCurrentTimeMillis();
	float *pCaptureBuffer, *pRenderBuffer;
	//Used to temporarily store sample(for all out channels) while they are being processed.
	double tmpBuffer[8];
	//The size of all sample frames for all channels with the same sample index/timestamp
	const size_t renderBlockSize = sizeof(double) * _nChannelsOut;

	//Run infinite capture loop
	while (true) {
		//Check for samples in capture buffer
		assert(_pCaptureDevice->getNextPacketSize(&numFramesAvailable));

		if (numFramesAvailable != 0) {
			//Get capture buffer pointer and number of available frames.
			assert(_pCaptureDevice->getCaptureBuffer(&pCaptureBuffer, &numFramesAvailable));

			//Must read entire capture buffer at once. Wait until render buffer has enough space available.
			while (numFramesAvailable > _pRenderDevice->getBufferFrameCountAvailable()) {
				//Short sleep just to not busy wait all resources.
				Date::sleepMillis(1);
			}

			//Get render buffer
			assert(_pRenderDevice->getRenderBuffer(&pRenderBuffer, numFramesAvailable));

			//Get current timestamp to mark playing channels with
			now = Date::getCurrentTimeMillis();

			//Iterate all capture frames
			for (i = 0; i < numFramesAvailable; ++i) {
				//Set default value to 0 so we can add/mix values to it later
				memset(tmpBuffer, 0, renderBlockSize);

				//Iterate inpputs
				for (j = 0; j < _nChannelsIn; ++j) {
					//Identify which channels are playing.
					if (pCaptureBuffer[j] != 0) {
						_pUsedChannels[j] = now;
					}

					//Route sample to outputs
					_inputs[j]->route(pCaptureBuffer[j], tmpBuffer);
				}

				//Iterate outputs
				for (j = 0; j < _nChannelsOut; ++j) {
					//Apply output forks and filters
					pRenderBuffer[j] = (float)_outputs[j]->process(tmpBuffer[j]);

					//Check for digital clipping
					if (abs(pRenderBuffer[j]) > 1.0) {
						_pClippingChannels[j] = max(_pClippingChannels[j], abs(pRenderBuffer[j]));
					}
				}

				//Move buffers to next sample
				pCaptureBuffer += _nChannelsIn;
				pRenderBuffer += _nChannelsOut;
			}

			//Release / flush buffers
			assert(_pCaptureDevice->releaseCaptureBuffer(numFramesAvailable));
			assert(_pRenderDevice->releaseRenderBuffer(numFramesAvailable));
		}

		if (Date::getCurrentTimeMillis() - lastConfig > 1000) {
			//Check if config file has changed
			checkConfig();
			//Check for unused channels
			checkUsedChannels();
			//Check for clipping out channels
			checkClippingChannels();
			//Store timestamp
			lastConfig = Date::getCurrentTimeMillis();
		}
		else {
			//Check for samples in capture buffer
			assert(_pCaptureDevice->getNextPacketSize(&numFramesAvailable));
			//Short sleep just to not busy wait all resources.
			if (numFramesAvailable == 0) {
				Date::sleepMillis(1);
			}
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

void CaptureLoop::checkUsedChannels() {
	const time_t now = Date::getCurrentTimeMillis();
	//Compare now against last used timestamp and determine active channels
	for (size_t i = 0; i < _nChannelsIn; ++i) {
		if (now - _pUsedChannels[i] > 1000) {
			_pUsedChannels[i] = 0;
		}
	}
	//Update conditional routing
	for (const Input *pInut : _inputs) {
		pInut->evalConditions();
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