#include "CaptureLoop.h"

CaptureLoop::CaptureLoop(Config *pConfig, const AudioDevice *pRenderDevice, IAudioRenderClient *pRenderClient, IAudioCaptureClient *pCaptureClient, const std::vector<Input*> &inputs, const std::vector<Output*> &outputs) {
	_pConfig = pConfig;
	_pRenderDevice = pRenderDevice;
	_pRenderClient = pRenderClient;
	_pCaptureClient = pCaptureClient;
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
	HRESULT hr;
	DWORD flags;
	time_t now;
	float *pCaptureBuffer, *pRenderBuffer;
	size_t loopCounter = 0;
	size_t renderBlockSize = sizeof(float) * _nChannelsOut;

	//Run infinite capture loop
	while (true) {
		//Check for samples in capture buffer
		hr = _pCaptureClient->GetNextPacketSize(&numFramesAvailable);
		assertHrResult(hr);

		while (numFramesAvailable != 0) {
			//Get capture buffer pointer and number of available frames.
			hr = _pCaptureClient->GetBuffer((BYTE**)&pCaptureBuffer, &numFramesAvailable, &flags, NULL, NULL);
			assertHrResult(hr);

			//Must read entire capture buffer at once. Wait until render buffer has enough space available.
			while (numFramesAvailable > _pRenderDevice->getBufferFrameCountAvailable()) {
				//Date::sleepMillis(1);
			}

			//Get render buffer
			hr = _pRenderClient->GetBuffer(numFramesAvailable, (BYTE**)&pRenderBuffer);
			assertHrResult(hr);

			//Set default value to 0 so we can add/mix values to it later
			memset(pRenderBuffer, 0, numFramesAvailable * renderBlockSize);

			//Get current timestamp to mark playing channels with
			now = Date::getCurrentTimeMillis();

			//Iterate all capture frames
			for (i = 0; i < numFramesAvailable; ++i) {
				for (j = 0; j < _nChannelsIn; ++j) {
					//Identify which channels are playing.
					if (pCaptureBuffer[j] != 0) {
						_pUsedChannels[j] = now;
					}

					//Route sample to outputs
					_inputs[j]->route(pCaptureBuffer[j], pRenderBuffer);
				}
				
				for (j = 0; j < _nChannelsOut; ++j) {
					//Apply output filters
					pRenderBuffer[j] = (float)_outputs[j]->process(pRenderBuffer[j]);
					
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
			hr = _pCaptureClient->ReleaseBuffer(numFramesAvailable);
			assertHrResult(hr);
			hr = _pRenderClient->ReleaseBuffer(numFramesAvailable, 0);
			assertHrResult(hr);

			//Check for more samples in capture buffer
			hr = _pCaptureClient->GetNextPacketSize(&numFramesAvailable);
			assertHrResult(hr);
		}

		if (loopCounter == 1000) {
			loopCounter = 0;
			//Check if config file has changed
			checkConfig();
			//Check for unused channels
			checkUsedChannels();
			//Check for clipping out channels
			checkClippingChannels();
		}
		++loopCounter;

		//Short sleep just to not busy wait all resources.
		Date::sleepMillis(1);
	}
}