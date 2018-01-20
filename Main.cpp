#include "OS.h"
#include "CaptureLoop.h"
#include "MemoryManager.h"

#define VERSION "0.7.0b"

#ifdef DEBUG
#define PROFILE " - DEBUG mode"
#else
#define PROFILE ""
#endif

Config *pConfig = nullptr;
CaptureLoop *pLoop = nullptr;

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
	delete pLoop;
	pLoop = nullptr;
	AudioDevice::destroyStatic();
	JsonNode::destroyStatic();
#ifdef DEBUG_MEMORY
	MemoryManager::getInstance()->assertNoLeak();
#endif
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

	//Start capture loop.
	pLoop = new CaptureLoop(pConfig, pRenderDevice, pRenderClient, pCaptureClient, inputs, outputs);
	pLoop->capture();

	pCaptureDevice->stop();
	pRenderDevice->stop();
}

int main(int argc, char **argv) {
	printf("----------------------------------------------\n");
	printf("\tWinDSP %s%s\n", VERSION, PROFILE);
	printf("----------------------------------------------\n\n");
	
	OS::setPriorityHigh();

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






