/*
	This file contains the start of the application.
	From here all resources are initalized and the service starts.

	Author: Andreas Arvidsson
	Source: https://github.com/AndreasArvidsson/WinDSP
*/

#include "OS.h"
#include "CaptureLoop.h"
#include "AsioDevice.h"

#ifdef DEBUG
#include "MemoryManager.h"
#define PROFILE " - DEBUG mode"
#else
#define PROFILE ""
#endif

#define VERSION "0.14.0b"
#define TITLE_SIZE 64

char configFileNumber = '0';
Config *pConfig = nullptr;
char title[TITLE_SIZE];
//CaptureLoop *pLoop = nullptr;

void setVisibility() {
	if (pConfig->hide()) {
		OS::hideWindow();
	}
	else if (pConfig->minimize()) {
		OS::minimizeWindow();
	}
	else {
		OS::showWindow();
	}
}

void setTitle() {
	snprintf(title, TITLE_SIZE, "WinDSP %s%s", VERSION, PROFILE);
	SetConsoleTitle(title);
	printf("----------------------------------------------\n");
	printf("\t%s\n", title);
	printf("----------------------------------------------\n\n");
}

const std::string getConfigFileName() {
	if (configFileNumber != '0') {
		return std::string("WinDSP-") + configFileNumber + ".json";
	}
	else {
		return "WinDSP.json";
	}
}

const bool checkInput(const char input) {
	if (input) {
		configFileNumber = input;
		printf("Switching to config file '%s'\n\n", getConfigFileName().c_str());
		return true;
	}
	return false;
}

void clearData() {
	delete pConfig;
	pConfig = nullptr;
	Capture::destroy();
	AudioDevice::destroyStatic();
	AsioDevice::destroyStatic();
	JsonNode::destroyStatic();
#ifdef DEBUG_MEMORY
	//Check for memory leaks
	if (MemoryManager::getInstance()->hasLeak()) {
		OS::showWindow();
		printf("\n");
		MemoryManager::getInstance()->assertNoLeak();
	}
#endif
}

void run(const std::string &configName) {
	//Load config file
	pConfig = new Config();
	pConfig->init(configName);

	//Show or hide window
	setVisibility();

	/*
	* Get capture and render devices
	*/

	std::vector<AudioDevice*> devices = pConfig->getDevices();
	AudioDevice *pCaptureDevice = devices[0];
	AudioDevice *pRenderDevice = devices[1];

	printf("----------------------------------------------\n");
	printf("Starting DSP service @ %s\n", Date::getLocalDateTimeString().c_str());
	printf("Capture: %s\n", pCaptureDevice->getName().c_str());
	printf("Render: %s\n", pRenderDevice->getName().c_str());
	printf("----------------------------------------------\n\n");

	const WAVEFORMATEX *pCaptureFormat = pCaptureDevice->getFormat();
	const WAVEFORMATEX *pRenderFormat = pRenderDevice->getFormat();

	/*
	* Validate device settings
	*/

	//The application have no resampler. Sample rate and bit depth must be a match
	if (pCaptureFormat->nSamplesPerSec != pRenderFormat->nSamplesPerSec) {
		throw Error("Sample rate missmatch: Capture(%d), Render(%d))", pCaptureFormat->nSamplesPerSec, pRenderFormat->nSamplesPerSec);
	}
	if (pCaptureFormat->wBitsPerSample != pRenderFormat->wBitsPerSample) {
		throw Error("Bit depth missmatch: Capture(%d), Render(%d)", pCaptureFormat->wBitsPerSample, pRenderFormat->wBitsPerSample);
	}
	//Sample buffers must contains a 32bit float. All code depends on it
	if (pCaptureFormat->wBitsPerSample != 32) {
		throw Error("Bit depth doesnt match float(32), Found(%d)", pCaptureFormat->wBitsPerSample);
	}
	if (pCaptureFormat->wFormatTag != pRenderFormat->wFormatTag) {
		throw Error("Format tag missmatch: Capture(%d), Render(%d)", pCaptureFormat->wFormatTag, pRenderFormat->wFormatTag);
	}

	/*
	* Init I/O and filters
	*/

	//Read config and get I/O instances with filters 
	pConfig->init(pCaptureFormat->nSamplesPerSec, pCaptureFormat->nChannels, pRenderFormat->nChannels);
	std::vector<Input*> inputs = pConfig->getInputs();
	std::vector<Output*> outputs = pConfig->getOutputs();

	/*
	* Start capturing data
	*/

	//Start capture loop.

	const HWND windowHandle = FindWindow(NULL, title);
	AsioDevice *pRenderDeviceAsio = new AsioDevice("Focusrite USB ASIO", pRenderFormat->nChannels, windowHandle);
	//Capture::init(pConfig, &inputs, &outputs, pCaptureDevice, pRenderDevice);
	Capture::init(pConfig, &inputs, &outputs, pCaptureDevice, pRenderDeviceAsio);
	Capture::run();
	delete pRenderDeviceAsio;
}

int main(int argc, char **argv) {
	setTitle();

	//Flush denormalized zeroes
	OS::flushDenormalizedZero();
	//Set high priority on process
	OS::setPriorityHigh();

	for (;;) {
		try {
			//Run application
			run(getConfigFileName());
		}
		catch (const ConfigChangedException &e) {
			if (!checkInput(e.input)) {
				printf("Config file changed. Restarting...\n\n");
			}
		}
		//Keep trying for the service to come back
		catch (const std::exception &e) {
			OS::showWindow();
			printf("ERROR: %s\n\n", e.what());
			//Wait for device to possible come back after reconfigure
			Date::sleepMillis(1000);
			//Check keyboard input
			checkInput(Keyboard::getDigit());
		}
		//Release old resources
		clearData();
	}

	return EXIT_SUCCESS;
}






