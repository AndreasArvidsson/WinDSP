#include "CaptureLoop.h"
#include "AsioDevice.h"

#define MAX_INT32 2147483647.0

const Config *_pConfig;
const std::vector<Input*> *_pInputs;
const std::vector<Output*> *_pOutputs;
AudioDevice *_pCaptureDevice = nullptr;
AudioDevice *_pRenderDevice = nullptr;

bool *_pUsedChannels = nullptr;
float *_pClippingChannels = nullptr;
double *_renderBlockBuffer = nullptr;
size_t _nChannelsIn, _nChannelsOut = 0;

AsioDevice *_pRenderDeviceAsio = nullptr;
ASIOBufferInfo *_pBufferInfos = nullptr;
ASIOChannelInfo *_pChannelInfos = nullptr;
long _bufferSize = 0;
bool _postOutput = false;

void initInner() {
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

void Capture::init(const Config *pConfig, const std::vector<Input*> *pInputs, const std::vector<Output*> *pOutputs, AudioDevice *pCaptureDevice, AudioDevice *pRenderDevice) {
	_pConfig = pConfig;
	_pInputs = pInputs;
	_pOutputs = pOutputs;
	_pCaptureDevice = pCaptureDevice;
	_pRenderDevice = pRenderDevice;
	initInner();
}

void Capture::init(const Config *pConfig, const std::vector<Input*> *pInputs, const std::vector<Output*> *pOutputs, AudioDevice *pCaptureDevice, AsioDevice *pRenderDevice) {
	_pConfig = pConfig;
	_pInputs = pInputs;
	_pOutputs = pOutputs;
	_pCaptureDevice = pCaptureDevice;
	_pRenderDeviceAsio = pRenderDevice;
	initInner();
}

void Capture::destroy() {
	delete[] _pUsedChannels;
	delete[] _pClippingChannels;
	delete[] _renderBlockBuffer;
}


#include "SineGenerator.h"
SineGenerator singen(44100, 300);

float overflowBuffer[1024 * 20];
UINT32 overflowSize = 0;
int* pRenderBuffer;
UINT32 channelIndex, sampleIndex;

void writeToBuffer(const float *pSource, const UINT32 length, const int bufferIndex) {
	for (channelIndex = 0; channelIndex < _nChannelsOut; ++channelIndex) {
		pRenderBuffer = pRenderBuffer = (int*)_pBufferInfos[channelIndex].buffers[bufferIndex];
		for (sampleIndex = 0; sampleIndex < length; ++sampleIndex) {
			pRenderBuffer[sampleIndex] = (int)(MAX_INT32 * pSource[sampleIndex * _nChannelsIn + channelIndex]);
		}
	}
}

ASIOTime* asioBufferSwitchTimeInfo(ASIOTime* const pTimeInfo, const long bufferIndex, const ASIOBool processNow) {
	UINT32 numFramesAvailable, length, framesLeft;
	float *pCaptureBuffer;
	DWORD flags;

	for (int i = 0; i < _bufferSize; ++i) {
		double value = singen.next();
		for (int j = 0; j < _nChannelsIn; ++j) {
			overflowBuffer[i * _nChannelsIn + j] = value;
		}
	}
	writeToBuffer(overflowBuffer, _bufferSize, bufferIndex);
	return nullptr;

	//for (int i = 0; i < _bufferSize; ++i) {
	//	int value = singen.next() * MAX_INT32;
	//	for (int j = 0; j < _nChannelsOut; ++j) {
	//		pRenderBuffer = (int*)_pBufferInfos[j].buffers[bufferIndex];
	//		pRenderBuffer[i] = value;
	//	}
	//}


	//

	//printf("asioBufferSwitchTimeInfo %d\n", processNow);

	return nullptr;

	framesLeft = _bufferSize;

	//Have data leftover from last call. Process these first.
	if (overflowSize) {
		length = min(overflowSize, framesLeft);
		writeToBuffer(overflowBuffer, length, bufferIndex);
		overflowSize -= length;
		framesLeft -= length;
	}

	while (framesLeft > 0) {
		//Check for samples in capture buffer.
		assert(_pCaptureDevice->getNextPacketSize(&numFramesAvailable));
		if (numFramesAvailable) {
			//Get capture buffer pointer and number of available frames.
			assert(_pCaptureDevice->getCaptureBuffer(&pCaptureBuffer, &numFramesAvailable, &flags));

			length = min(numFramesAvailable, framesLeft);
			writeToBuffer(pCaptureBuffer, length, bufferIndex);

			//Data left in capture buffer. Store in overflow.
			if (numFramesAvailable > length) {
				overflowSize = numFramesAvailable - length;
				memcpy(overflowBuffer, pCaptureBuffer + length * _nChannelsIn, overflowSize * _nChannelsIn * sizeof(float));
			}

			framesLeft -= length;

			//Release/flush capture buffers.
			assert(_pCaptureDevice->releaseCaptureBuffer(numFramesAvailable));
		}
		else {
			//Short sleep just to not busy wait all resources.
			Date::sleepMillis(1);
		}
	}

	/*		int a = _pCaptureDevice->getBufferFrameCount();
		int b = _pCaptureDevice->getBufferFrameCountAvailable();
		int c = 2;*/

	if (_postOutput) {
		assertAsio(ASIOOutputReady());
	}

	return nullptr;
}

void asioBufferSwitch(long index, ASIOBool processNow) {
	ASIOTime timeInfo{};
	assertAsio(ASIOGetSamplePosition(&timeInfo.timeInfo.samplePosition, &timeInfo.timeInfo.systemTime));
	asioBufferSwitchTimeInfo(&timeInfo, index, processNow);
}

long asioMessage(long selector, long value, void* message, double* opt) {
	//Must be done here for thread purposes.
	if (!_pBufferInfos) {
		_pBufferInfos = _pRenderDeviceAsio->getBufferInfos();
		_pChannelInfos = _pRenderDeviceAsio->getChannelsInfos();
		_bufferSize = _pRenderDeviceAsio->getBufferSize();
		_postOutput = _pRenderDeviceAsio->getPostOutput();
	}

	switch (selector) {
	case kAsioSelectorSupported:
	case kAsioResetRequest:
	case kAsioResyncRequest:
	case kAsioSupportsTimeInfo:
	case kAsioLatenciesChanged:
		return 1L;
	case kAsioEngineVersion:
		return 2L;
	}
	return 0L;
}

void startDevices() {
	_pCaptureDevice->startCaptureService();
	if (_pRenderDevice) {
		_pRenderDevice->startRenderService();
	}
	if (_pRenderDeviceAsio) {
		_pRenderDeviceAsio->printInfo();
		ASIOCallbacks callbacks{ 0 };
		callbacks.asioMessage = asioMessage;
		callbacks.bufferSwitch = &asioBufferSwitch;
		callbacks.bufferSwitchTimeInfo = &asioBufferSwitchTimeInfo;
		_pRenderDeviceAsio->startRenderService(&callbacks);
	}
}

void Capture::run() {
	UINT32 i, j, numFramesAvailable;
	float *pCaptureBuffer, *pRenderBuffer;
	DWORD flags;
	//The size of all sample frames for all channels with the same sample index/timestamp
	const size_t renderBlockSize = sizeof(double) * _nChannelsOut;
	time_t lastCritical = 0;;
	time_t lastNotCritical = 0;
	bool clippingDetected = false;
	bool silent = true;

	//Initialize audio devices and start services
	startDevices();

	if (_pRenderDeviceAsio) {
		for (;;);
	}

	printf("After\n");

	for (;;) {
		//Wait for device callback
		assertWait(WaitForSingleObjectEx(_pRenderDevice->getEventHandle(), INFINITE, false));

		//Check for samples in capture buffer.
		assert(_pCaptureDevice->getNextPacketSize(&numFramesAvailable));

		//No data in capture device. Just continue.
		if (!numFramesAvailable) {
			continue;
		}

		//Must read entire capture buffer at once. Wait until render buffer has enough space available.
		while (numFramesAvailable > _pRenderDevice->getBufferFrameCountAvailable()) {
			//Short sleep just to not busy wait all resources.
			Date::sleepMillis(1);
		}

		//Get capture buffer pointer and number of available frames.
		assert(_pCaptureDevice->getCaptureBuffer(&pCaptureBuffer, &numFramesAvailable, &flags));

		//Get render buffer for size.
		assert(_pRenderDevice->getRenderBuffer(&pRenderBuffer, numFramesAvailable));

		UINT32 numChannels = (UINT32)min(_nChannelsIn, _nChannelsOut);
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