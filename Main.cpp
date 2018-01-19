#include "Config.h"
#include "Date.h"
#include "ConfigChangedException.h"
#include "MemoryManager.h"
#include "OS.h"

#include  <algorithm>
#undef max

#define VERSION "0.6.0b"

#ifdef DEBUG
#define PROFILE " - DEBUG mode"
#else
#define PROFILE ""
#endif

Config *pConfig = nullptr;
int loopCounter = 0;

inline void checkConfig() {
	if (loopCounter > 1000) {
		loopCounter = 0;
		if (pConfig->hasChanged()) {
			throw ConfigChangedException();
		}
	}
	++loopCounter;
}

void setVisibility() {
	if (pConfig->minimize()) {
		OS::minimize();
	}
	else {
		OS::show();
	}
}

void clearData() {
	delete pConfig;
	pConfig = nullptr;
	AudioDevice::destroyStatic();
	JsonNode::destroyStatic();

#ifdef DEBUG_MEMORY
	MemoryManager::getInstance()->assertNoLeak();
#endif
}

void fill(std::vector<float> &vec) {
	for (size_t i = 0; i < vec.size(); ++i) {
		vec[i] = 0;
	}
}

void loop(const AudioDevice *pRenderDevice, IAudioRenderClient *pRenderClient, IAudioCaptureClient *pCaptureClient,
	const WAVEFORMATEX *pCaptureFormat, const WAVEFORMATEX *pRenderFormat, const std::vector<Input*> &inputs, const std::vector<Output*> &outputs) {
	HRESULT hr;
	DWORD flags;
	UINT32 packetLength, numFramesAvailable, i, j;
	float *pCaptureBuffer, *pRenderBuffer;
	const UINT32 nChannelsIn = pCaptureFormat->nChannels;
	const UINT32 nChannelsOut = pRenderFormat->nChannels;
	const UINT32 renderBlockSize = sizeof(float) * nChannelsOut;

	std::vector<float> inputLevels(nChannelsIn);
	std::vector<float> outputLevels(nChannelsOut);

	while (true) {
		hr = pCaptureClient->GetNextPacketSize(&packetLength);
		assertHrResult(hr);

#ifdef DEBUG
		fill(inputLevels);
		fill(outputLevels);
#endif

		while (packetLength != 0) {
			//Get capture buffer pointer and number of available frames.
			hr = pCaptureClient->GetBuffer((BYTE**)&pCaptureBuffer, &numFramesAvailable, &flags, NULL, NULL);
			assertHrResult(hr);

			//Must read entire capture buffer at once. Wait until render buffer has enough space available.
			while (numFramesAvailable > pRenderDevice->getBufferFrameCountAvailable()) {
				//Date::sleepMillis(1);
			}

			//Get render buffer
			hr = pRenderClient->GetBuffer(numFramesAvailable, (BYTE**)&pRenderBuffer);
			assertHrResult(hr);

			//Set default value to 0 so we can add/mix values to it later
			memset(pRenderBuffer, 0, numFramesAvailable * renderBlockSize);

			//Iterate all capture frames
			for (i = 0; i < numFramesAvailable; ++i) {
				//Iterate each input and route to output
				for (j = 0; j < inputs.size(); ++j) {

					inputLevels[j] = std::max(inputLevels[j], std::abs(pCaptureBuffer[j]));

					//Process/route sample
					inputs[j]->process(pCaptureBuffer[j], pRenderBuffer);
				}
				//Iterate each output and apply filters
				for (j = 0; j < outputs.size(); ++j) {
					pRenderBuffer[j] = (float)outputs[j]->process(pRenderBuffer[j]);

					outputLevels[j] = std::max(outputLevels[j], std::abs(pRenderBuffer[j]));

				}

				pCaptureBuffer += nChannelsIn;
				pRenderBuffer += nChannelsOut;
			}

			//Release / flush buffers
			hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
			assertHrResult(hr);

			hr = pRenderClient->ReleaseBuffer(numFramesAvailable, 0);
			assertHrResult(hr);

			hr = pCaptureClient->GetNextPacketSize(&packetLength);
			assertHrResult(hr);
		}

#ifdef DEBUG
		if (loopCounter % 100 == 0) {
			printf("\r");
			for (size_t i = 0; i < inputLevels.size(); ++i) {
				printf("%.2f\t", 20 * log10(inputLevels[i] / sqrt(2)));
				//\033[A
			}
		}
#endif

		//Check if config file has changed
		checkConfig();

		//Short sleep just to not busy wait all resources.
		Date::sleepMillis(1);
	}
}

void run() {
	/*
	* Get capture and render devices
	*/

	std::vector<AudioDevice*> devices = pConfig->getDevices();
	AudioDevice *pCaptureDevice = devices[0];
	AudioDevice *pRenderDevice = devices[1];

	printf("----------------------------------------------\n");
	printf("Starting capture service\n");
	printf("Capture: %s\n", pCaptureDevice->getName().c_str());
	printf("Render: %s\n", pRenderDevice->getName().c_str());
	printf("----------------------------------------------\n\n");

	const WAVEFORMATEX *pCaptureFormat = pCaptureDevice->getFormat();
	const WAVEFORMATEX *pRenderFormat = pRenderDevice->getFormat();

	//Validate device settings
	if (pCaptureFormat->nSamplesPerSec != pRenderFormat->nSamplesPerSec) {
		throw Error("Sample rate missmatch: Capture(%d), Render(%d))", pCaptureFormat->nSamplesPerSec, pRenderFormat->nSamplesPerSec);
	}
	if (pCaptureFormat->wBitsPerSample != pRenderFormat->wBitsPerSample) {
		throw Error("Bit depth missmatch: Capture(%d), Render(%d)", pCaptureFormat->wBitsPerSample, pRenderFormat->wBitsPerSample);
	}
	if (pCaptureFormat->wBitsPerSample != 32) {
		throw Error("Bit depth doesnt match float(32), Found(%d)", pCaptureFormat->wBitsPerSample);
	}
	if (pCaptureFormat->wFormatTag != pRenderFormat->wFormatTag) {
		throw Error("Format tag missmatch: Capture(%d), Render(%d)", pCaptureFormat->wFormatTag, pRenderFormat->wFormatTag);
	}

	IAudioRenderClient *pRenderClient = pRenderDevice->getRenderClient();
	IAudioCaptureClient *pCaptureClient = pCaptureDevice->getCaptureClient();

	/*
	* Init I/O and filters
	*/

	pConfig->init(pCaptureFormat->nSamplesPerSec, pCaptureFormat->nChannels, pRenderFormat->nChannels);
	std::vector<Input*> inputs = pConfig->getInputs();
	std::vector<Output*> outputs = pConfig->getOutputs();

	/*
	* Start capturing data
	*/

	//Get and release render buffer first time. Needed to not get audio glitches
	BYTE *pRenderBuffer;
	HRESULT hr = pRenderClient->GetBuffer(pRenderDevice->getBufferFrameCount(), &pRenderBuffer);
	assertHrResult(hr);
	hr = pRenderClient->ReleaseBuffer(pRenderDevice->getBufferFrameCount(), AUDCLNT_BUFFERFLAGS_SILENT);
	assertHrResult(hr);

	//Show or hide window
	setVisibility();

	pCaptureDevice->start();
	pRenderDevice->start();

	loop(pRenderDevice, pRenderClient, pCaptureClient, pCaptureFormat, pRenderFormat, inputs, outputs);

	pCaptureDevice->stop();
	pRenderDevice->stop();
}

int main(int argc, char **argv) {
	printf("----------------------------------------------\n");
	printf("\tWinDSP %s%s\n", VERSION, PROFILE);
	printf("----------------------------------------------\n\n");

	OS::setPriorityHigh();
	//Start capture service.
	while (true) {
		try {
			pConfig = new Config();
			pConfig->init("WinDSP.json");
			run();
		}
		catch (const ConfigChangedException) {
			printf("Config file changed. Restarting...\n\n");
		}
		//Keep trying for the service to come back
		catch (const std::exception &e) {
			OS::show();
			printf("ERROR: %s\n\n", e.what());
			//Wait for device to possible come back after reconfigure
			Date::sleepMillis(1000);
		}
		//Release old resources
		clearData();
	}

	return EXIT_SUCCESS;
}







//t1 = Date::getCurrentTimeMillis();
//Sleep for half the buffer length.
//size_t sleepTime = pRenderDevice->getBufferFrameCount() * 1000 / pCaptureFormat->nSamplesPerSec / 2;
//Sleep for half buffer length. Subtract processing time.		
//Date::sleepMillis(sleepTime - (Date::getCurrentTimeMillis() - t1));

//const UINT32 captureBlockSize = sizeof(float) * nChannelsIn;
//Silence or packets in wrong order. Use silence.
//if (flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY || flags & AUDCLNT_BUFFERFLAGS_SILENT) {
//	memset(pCaptureBuffer, 0, numFramesAvailable * captureBlockSize);
//}

