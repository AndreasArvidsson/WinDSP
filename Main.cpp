/*
	This file contains the start of the application.
	From here all resources are initalized and the service starts.

	Author: Andreas Arvidsson
	Source: https://github.com/AndreasArvidsson/WinDSP
*/

#include "OS.h"
#include "CaptureLoop.h"
#include "WinDSPLog.h"
#include "Keyboard.h"
#include "Date.h"
#include "ConfigChangedException.h"
#include "Config.h"
#include "resource.h"
#include "JsonNode.h"
#include "AudioDevice.h"
#include "Visibility.h"
#include "TrayIcon.h"
#include "Str.h"
#include "AsioDevice.h"

#define VERSION "0.21.0b"

#ifdef DEBUG
#include "MemoryManager.h"
#define PROFILE " - DEBUG profile"
#else
#define PROFILE ""
#endif

char configFileNumber = '0';
Config *pConfig = nullptr;
AudioDevice *pCaptureDevice = nullptr;
AudioDevice *pRenderDevice = nullptr;
CaptureLoop *pCaptureLoop = nullptr;

LONG_PTR CALLBACK trayIconCallback(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
	if (iMsg == TRAY_ICON_MSG && lParam == WM_LBUTTONDBLCLK) {
        Visibility::show(true);
		return 0; 
	}
	return DefWindowProc(hwnd, iMsg, wParam, lParam);
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

const std::string getTitle() {
    return String::format("WinDSP %s%s", VERSION, PROFILE);
}

void setTitle() {
    const std::string title = getTitle();
	SetConsoleTitle(title.c_str());
	TrayIcon::init(trayIconCallback, IDI_ICON1, title);
}

void logTitle() {
    LOG_NL();
    LOG_INFO("----------------------------------------------");
    LOG_INFO("\t%s", getTitle().c_str());
#ifdef DEBUG_LOG
    LOG_INFO("\tLog file: %s", LOG_FILE);
#endif
    LOG_INFO("----------------------------------------------");
    LOG_NL();
}

const std::string getConfigFileName() {
	if (configFileNumber != '0') {
        return String::format("WinDSP-%c.json", configFileNumber);
	}
	else {
		return "WinDSP.json";
	}
}

const bool checkInput(const char input) {
	if (input) {
		configFileNumber = input;
		LOG_INFO("Switching to config file '%s'", getConfigFileName().c_str());
        LOG_NL();
		return true;
	}
	return false;
}

void clearData() {
    if (pCaptureLoop) {
        pCaptureLoop->stop();
    }
    if (pConfig->useAsioRenderDevice()) {
        AsioDevice::stopService();
        AsioDevice::destroy();
    }
    delete pCaptureLoop;
	delete pConfig;
	delete pCaptureDevice;
	delete pRenderDevice;
	pConfig = nullptr;
	pCaptureDevice = nullptr;
	pRenderDevice = nullptr;
	AudioDevice::destroyStatic();
	JsonNode::destroyStatic();
    WinDSPLog::destroyStatic();

#ifdef DEBUG_MEMORY
	//Check for memory leaks
	if (MemoryManager::getInstance()->hasLeak()) {
		Visibility::show(true);
		MemoryManager::getInstance()->assertNoLeak();
	}
#endif
}

void run() {
	const std::string configPath = OS::getExeDirPath() + getConfigFileName();

    //Init log with this thread.
    WinDSPLog::init();

	//Load config file
	pConfig = new Config(configPath);

    //Log title to console.
    logTitle();

	//Update start with OS.
	updateStartWithOS();

	//Show or hide window
	Visibility::update(pConfig);


	/*
	* Get capture and render devices
	*/

	const std::string captureDeviceName = pConfig->getCaptureDeviceName();
	const std::string renderDeviceName = pConfig->getRenderDeviceName();


	/*
	 * Create and initalize devices and validate device settings
	 */

	pCaptureDevice = AudioDevice::initDevice(captureDeviceName);
	pCaptureDevice->initCaptureService();
	const WAVEFORMATEX * const pCaptureFormat = pCaptureDevice->getFormat();
    //Sample buffers must contains a 32bit float. All code depends on it.
    if (pCaptureFormat->wBitsPerSample != 32) {
        throw Error("Bit depth doesnt match float(32), Found(%d)", pCaptureFormat->wBitsPerSample);
    }

	uint32_t renderNumChannels;

    //ASIO render device.
    if (pConfig->useAsioRenderDevice()) {
        renderNumChannels = pConfig->getAsioNumChannels() > 0 ? pConfig->getAsioNumChannels() : pCaptureDevice->getFormat()->nChannels;
        AsioDevice::initRenderService(pConfig, renderDeviceName, pCaptureFormat->nSamplesPerSec, pConfig->getAsioBufferSize(), renderNumChannels);
    }
    //WASAPI render device.
    else {
        pRenderDevice = AudioDevice::initDevice(renderDeviceName);
        pRenderDevice->initRenderService();
        const WAVEFORMATEX * const pRenderFormat = pRenderDevice->getFormat();
        renderNumChannels = pRenderFormat->nChannels;
        //The application have no resampler. Sample rate and bit depth must be a match.
        if (pCaptureFormat->nSamplesPerSec != pRenderFormat->nSamplesPerSec) {
            throw Error("Sample rate missmatch: Capture(%d), Render(%d)", pCaptureFormat->nSamplesPerSec, pRenderFormat->nSamplesPerSec);
        }
        if (pCaptureFormat->wBitsPerSample != pRenderFormat->wBitsPerSample) {
            throw Error("Bit depth missmatch: Capture(%d), Render(%d)", pCaptureFormat->wBitsPerSample, pRenderFormat->wBitsPerSample);
        }
        if (pCaptureFormat->wFormatTag != pRenderFormat->wFormatTag) {
            throw Error("Format tag missmatch: Capture(%d), Render(%d)", pCaptureFormat->wFormatTag, pRenderFormat->wFormatTag);
        }
    }

	/*
	* Init I/O and filters
	*/


	//Read config and get I/O instances with filters 
	pConfig->init(pCaptureFormat->nSamplesPerSec, pCaptureFormat->nChannels, renderNumChannels);


    /*
     * Print data to the user.
     */

    const std::string renderPrefix = pConfig->useAsioRenderDevice() ? " (ASIO)" : "";
    LOG_INFO("----------------------------------------------");
    LOG_INFO("Starting DSP service @ %s", Date::getLocalDateTimeString().c_str());
    LOG_INFO("Capture: %s", captureDeviceName.c_str());
    LOG_INFO("Render%s: %s", renderPrefix.c_str(), renderDeviceName.c_str());
    if (pConfig->inDebug() && pConfig->useAsioRenderDevice()) {
        LOG_INFO("ASIO buffer: %d(%.1fms)", AsioDevice::getBufferSize(), 1000.0 * AsioDevice::getBufferSize() / pCaptureFormat->nSamplesPerSec);
    }
    if (pConfig->hasDescription()) {
        LOG_INFO("%s", pConfig->getDescription().c_str());
    }
    LOG_INFO("----------------------------------------------");
    LOG_NL();
    if (pConfig->inDebug()) {
        pConfig->printConfig();
    }


	/*
	* Start capturing data
	*/

    pCaptureLoop = new CaptureLoop(pConfig, pCaptureDevice, pRenderDevice);
    pCaptureLoop->run();
}

void main() {
	setTitle();

	//Very important for filter performance.
	OS::setPriorityHigh();

	size_t waiting = 0;
	for (;;) {
		try {
			//Run application
			run();
		}
		catch (const ConfigChangedException &e) {
            WinDSPLog::flush();
			if (!checkInput(e.getInput())) {
				LOG_INFO("Config file changed. Restarting...");
                LOG_NL();
			}
		}
		//Keep trying for the service to come back
		catch (const std::exception &e) {
			Visibility::show(true);
            WinDSPLog::flush();
			LOG_ERROR("ERROR: %s", e.what());

			//Wait before trying again.
			waiting = 20;
		}

		//Release old resources.
		clearData();

		//Wait for device to possible come back after reconfigure.
		while (waiting > 0) {
			--waiting;

			//Sleep not to busy wait all resources.
			Date::sleepMillis(100);

			//Check keyboard input. Needed if user want to change config during error state.
			checkInput(Keyboard::getDigit());
		}
	}
}