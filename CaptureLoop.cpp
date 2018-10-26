#include "CaptureLoop.h"
#include "AsioDevice.h"

#define MAX_INT32 2147483647.0 //TODO

const Config *CaptureLoop::_pConfig;
const std::vector<Input*> *CaptureLoop::_pInputs;
const std::vector<Output*> *CaptureLoop::_pOutputs;
AudioDevice *CaptureLoop::_pCaptureDevice = nullptr;
AudioDevice *CaptureLoop::_pRenderDevice = nullptr;
bool *CaptureLoop::_pUsedChannels = nullptr;
float *CaptureLoop::_pClippingChannels = nullptr;
float CaptureLoop::_pOverflowBuffer[1024 * 8];
double *CaptureLoop::_renderBlockBuffer = nullptr;
size_t CaptureLoop::_nChannelsIn = 0;
size_t CaptureLoop::_nChannelsOut = 0;
UINT32 CaptureLoop::_overflowSize = 0;

void CaptureLoop::init(const Config *pConfig, const std::vector<Input*> *pInputs, const std::vector<Output*> *pOutputs, AudioDevice *pCaptureDevice, AudioDevice *pRenderDevice) {
	_pRenderDevice = pRenderDevice;
	init(pConfig, pInputs, pOutputs, pCaptureDevice);
}

void CaptureLoop::init(const Config *pConfig, const std::vector<Input*> *pInputs, const std::vector<Output*> *pOutputs, AudioDevice *pCaptureDevice) {
	_pConfig = pConfig;
	_pInputs = pInputs;
	_pOutputs = pOutputs;
	_pCaptureDevice = pCaptureDevice;
	_nChannelsIn = _pInputs->size();
	_nChannelsOut = _pOutputs->size();

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

void CaptureLoop::destroy() {
	delete[] _pUsedChannels;
	delete[] _pClippingChannels;
	delete[] _renderBlockBuffer;
}

void CaptureLoop::run() {
	_pCaptureDevice->startCaptureService();
	//Wasapi render device
	if (_pRenderDevice) {
		_pRenderDevice->startRenderService();
		__wasapiLoop();
	}
	//Asio render device
	else {
		ASIOCallbacks callbacks{ 0 };
		callbacks.bufferSwitch = &__bufferSwitch;
		AsioDevice::printInfo();
		AsioDevice::startRenderService(&callbacks);
		for (;;) {
			//Short sleep just to not busy wait all resources.
			Sleep(1);
		}
		//AsioDevice::stopRenderService();
	}
}

void CaptureLoop::__bufferSwitch(long bufferIndex, ASIOBool processNow) {
	UINT32 numFramesAvailable, length;
	float *pCaptureBuffer;
	DWORD flags;

	UINT32 framesLeft = AsioDevice::bufferSize;
	UINT32 offset = 0;

	//Have data leftover from last call. Process these first.
	if (_overflowSize > 0) {
		length = min(_overflowSize, framesLeft);
		__writeToBuffer(_pOverflowBuffer, length, bufferIndex);
		_overflowSize -= length;
		framesLeft -= length;
		offset += length;
	}

	while (framesLeft > 0) {
		//Check for samples in capture buffer.
		assert(_pCaptureDevice->getNextPacketSize(&numFramesAvailable));
		if (numFramesAvailable) {
			//Get capture buffer pointer and number of available frames.
			assert(_pCaptureDevice->getCaptureBuffer(&pCaptureBuffer, &numFramesAvailable, &flags));

			length = min(numFramesAvailable, framesLeft);
			__writeToBuffer(pCaptureBuffer, length, bufferIndex, offset);

			//Data left in capture buffer. Store in overflow.
			if (numFramesAvailable > length) {
				_overflowSize = numFramesAvailable - length;
				memcpy(_pOverflowBuffer, pCaptureBuffer + length * _nChannelsIn, _overflowSize * _nChannelsIn * sizeof(float));
			}

			framesLeft -= length;
			offset += length;

			//Release/flush capture buffers.
			assert(_pCaptureDevice->releaseCaptureBuffer(numFramesAvailable));
		}
		else {
			//Short sleep just to not busy wait all resources.
			Sleep(1);
		}
	}

	if (AsioDevice::outputReady) {
		assertAsio(ASIOOutputReady());
	}
}

void CaptureLoop::__writeToBuffer(const float* const pSource, const UINT32 length, const int bufferIndex, const UINT32 offset) {
	for (UINT32 channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
		int *pRenderBuffer = pRenderBuffer = (int*)AsioDevice::pBufferInfos[channelIndex].buffers[bufferIndex];
		for (UINT32 sampleIndex = 0; sampleIndex < length; ++sampleIndex) {
			pRenderBuffer[offset + sampleIndex] = (int)(MAX_INT32 * pSource[sampleIndex * _nChannelsIn + channelIndex]);
		}
	}
}

void CaptureLoop::__wasapiLoop() {
	UINT32 i, j, numFramesAvailable;
	float *pCaptureBuffer, *pRenderBuffer;
	DWORD flags;
	//The size of all sample frames for all channels with the same sample index/timestamp
	const size_t renderBlockSize = sizeof(double) * _nChannelsOut;
	time_t lastCritical = 0;
	time_t lastNotCritical = 0;
	bool clippingDetected = false;
	bool silent = true;
	for (;;) {
		//Wait for device callback
		assertWait(WaitForSingleObjectEx(_pRenderDevice->getEventHandle(), INFINITE, false));

		//Check for samples in capture buffer.
		assert(_pCaptureDevice->getNextPacketSize(&numFramesAvailable));

		//No data in capture device. Just continue.
		if (!numFramesAvailable) {
			//Short sleep just to not busy wait all resources.
			Sleep(1);
			continue;
		}

		//Must read entire capture buffer at once. Wait until render buffer has enough space available.
		while (numFramesAvailable > _pRenderDevice->getBufferFrameCountAvailable()) {
			//Short sleep just to not busy wait all resources.
			Sleep(1);
		}

		//Get capture buffer pointer and number of available frames.
		assert(_pCaptureDevice->getCaptureBuffer(&pCaptureBuffer, &numFramesAvailable, &flags));

		//Get render buffer for size.
		assert(_pRenderDevice->getRenderBuffer(&pRenderBuffer, numFramesAvailable));

		for (i = 0; i < numFramesAvailable; ++i) {
			for (j = 0; j < _nChannelsOut; ++j) {
				pRenderBuffer[j] = pCaptureBuffer[j];
			}
			//Move buffers to next sample
			pCaptureBuffer += _nChannelsIn;
			pRenderBuffer += _nChannelsOut;
		}

		//Release/flush capture buffers.
		assert(_pCaptureDevice->releaseCaptureBuffer(numFramesAvailable));
		assert(_pRenderDevice->releaseRenderBuffer(numFramesAvailable));
	}
}