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
AudioDevice *pCaptureDevice = nullptr;
AudioDevice *pRenderDevice = nullptr;
char title[TITLE_SIZE];

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
	CaptureLoop::stop();
	CaptureLoop::destroy();
	delete pConfig;
	delete pCaptureDevice;
	delete pRenderDevice;
	pConfig = nullptr;
	pCaptureDevice = nullptr;
	pRenderDevice = nullptr;
	AsioDevice::destroy();
	AudioDevice::destroyStatic();
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

void updateStartWithOS() {
	const HKEY hKey = HKEY_CURRENT_USER;
	const std::string path = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
	const std::string name = "WinDSP";
	//Should start with OS. Add to registry.
	if (pConfig->startWithOS()) {
		const std::string exePath = OS::getExePath();
		OS::regSetValue(hKey, path, name, exePath);
	}
	//Added to registry previusly. Remove.
	else if (OS::regValueExists(hKey, path, name)) {
		OS::regRemoveValue(hKey, path, name);
	}
}

void run() {
	//Load config file
	pConfig = new Config();
	pConfig->init(getConfigFileName());

	//Update start with OS.
	updateStartWithOS();

	//Show or hide window
	setVisibility();

	/*
	* Get capture and render devices
	*/

	const std::string captureDeviceName = pConfig->getCaptureDeviceName();
	const std::string renderDeviceName = pConfig->getRenderDeviceName();

	printf("----------------------------------------------\n");
	printf("Starting DSP service @ %s\n", Date::getLocalDateTimeString().c_str());
	printf("Capture(WASAPI): %s\n", captureDeviceName.c_str());
	if (!pConfig->useAsioRenderer()) {
		printf("Render(WASAPI): %s\n", renderDeviceName.c_str());
	}
	else {
		printf("Render(ASIO): %s\n", renderDeviceName.c_str());
	}
	printf("----------------------------------------------\n\n");

	/*
	 * Create and initalize devices and validate device settings
	 */

	pCaptureDevice = AudioDevice::initDevice(captureDeviceName);
	pCaptureDevice->initCaptureService();
	const WAVEFORMATEX *pCaptureFormat = pCaptureDevice->getFormat();
	uint32_t rendererSampleRate, renderNumChannels;

	if (pConfig->useAsioRenderer()) {
		AsioDevice::init(renderDeviceName, FindWindow(NULL, title));
		rendererSampleRate = AsioDevice::getSampleRate();
		renderNumChannels = pConfig->getNumChannelsRender(AsioDevice::getNumOutputChannels());
	}
	else {
		pRenderDevice = AudioDevice::initDevice(renderDeviceName);
		pRenderDevice->initRenderService();
		const WAVEFORMATEX *pRenderFormat = pRenderDevice->getFormat();
		rendererSampleRate = pRenderFormat->nSamplesPerSec;
		renderNumChannels = pRenderFormat->nChannels;

		if (pCaptureFormat->wBitsPerSample != pRenderFormat->wBitsPerSample) {
			throw Error("Bit depth missmatch: Capture(%d), Render(%d)", pCaptureFormat->wBitsPerSample, pRenderFormat->wBitsPerSample);
		}
		//Sample buffers must contains a 32bit float. All code depends on it.
		if (pCaptureFormat->wBitsPerSample != 32) {
			throw Error("Bit depth doesnt match float(32), Found(%d)", pCaptureFormat->wBitsPerSample);
		}
		if (pCaptureFormat->wFormatTag != pRenderFormat->wFormatTag) {
			throw Error("Format tag missmatch: Capture(%d), Render(%d)", pCaptureFormat->wFormatTag, pRenderFormat->wFormatTag);
		}
	}

	//The application have no resampler. Sample rate and bit depth must be a match.
	if (pCaptureFormat->nSamplesPerSec != rendererSampleRate) {
		throw Error("Sample rate missmatch: Capture(%d), Render(%d))", pCaptureFormat->nSamplesPerSec, rendererSampleRate);
	}


	/*
	* Init I/O and filters
	*/

	//Read config and get I/O instances with filters 
	pConfig->init(rendererSampleRate, pCaptureFormat->nChannels, renderNumChannels);


	/*
	* Start capturing data
	*/

	CaptureLoop::init(pConfig, pCaptureDevice, pRenderDevice);
	CaptureLoop::run();
}



int main(int argc, char **argv) {
	setTitle();

	//Very important for filter performance.
	OS::flushDenormalizedZero();
	OS::setPriorityHigh();

	size_t waiting = 0;
	for (;;) {
		try {
			//Run application
			run();
		}
		catch (const ConfigChangedException &e) {
			if (!checkInput(e.input)) {
				printf("Config file changed. Restarting...\n\n");
			}
		}
		//Keep trying for the service to come back
		catch (const std::exception &e) {
			//Release old resources before waiting.
			clearData();

			OS::showWindow();
			printf("ERROR: %s\n\n", e.what());

			//Wait before trying again.
			waiting = 20;
		}

		//Release old resources.
		clearData();

		//Wait for device to possible come back after reconfigure
		while (waiting > 0) {
			--waiting;

			//Sleep not to busy wait all resources.
			Date::sleepMillis(100);

			//Check keyboard input. Needed if user want to change config during error state.
			checkInput(Keyboard::getDigit());
		}
	}

	return EXIT_SUCCESS;
}






