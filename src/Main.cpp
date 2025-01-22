/*
    This file contains the start of the application.
    From here all resources are initalized and the service starts.

    Author: Andreas Arvidsson
    Source: https://github.com/AndreasArvidsson/WinDSP
*/

#include "../resource.h"
#include "OS.h"
#include "CaptureLoop.h"
#include "WinDSPLog.h"
#include "Keyboard.h"
#include "Date.h"
#include "ConfigChangedException.h"
#include "Config.h"
#include "JsonNode.h"
#include "AudioDevice.h"
#include "Visibility.h"
#include "TrayIcon.h"
#include "Str.h"
#include "AsioDevice.h"

using std::exception;
using std::make_shared;

#define VERSION "1.0.1"

#ifdef DEBUG
#include "MemoryManager.h"
#define PROFILE " - DEBUG profile"
#else
#define PROFILE ""
#endif

char configFileNumber = '0';
shared_ptr<Config> pConfig;

LONG_PTR CALLBACK trayIconCallback(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
    if (iMsg == TRAY_ICON_MSG && lParam == WM_LBUTTONDBLCLK) {
        Visibility::show(true);
        return 0;
    }
    return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

void updateStartWithOS() {
    const HKEY hKey = HKEY_CURRENT_USER;
    const string path = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    const string name = "WinDSP";
    // Should start with OS. Add to registry.
    if (pConfig->startWithOS()) {
        const string exePath = OS::getExePath();
        OS::regSetValue(hKey, path, name, exePath);
    }
    // Added to registry previusly. Remove.
    else if (OS::regValueExists(hKey, path, name)) {
        OS::regRemoveValue(hKey, path, name);
    }
}

const string getTitle() {
    return String::format("WinDSP %s%s", VERSION, PROFILE);
}

void setTitle() {
    const string title = getTitle();
    SetConsoleTitle(title.c_str());
    TrayIcon::init(trayIconCallback, IDI_ICON1, title);
}

void logTitle() {
    LOG_NL();
    LOG_INFO("----------------------------------------------");
    LOG_INFO("\t%s", getTitle().c_str());
    LOG_INFO("----------------------------------------------");
    LOG_NL();
}

const string getConfigFileName() {
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
    if (pConfig) {
        if (pConfig->useAsioRenderDevice()) {
            AsioDevice::stopService();
            AsioDevice::destroy();
        }
        pConfig = nullptr;
    }
    AudioDevice::destroyStatic();
    WinDSPLog::destroy();

#ifdef DEBUG_MEMORY
    // Check for memory leaks
    if (MemoryManager::getInstance()->hasLeak()) {
        Visibility::show(true);
        MemoryManager::getInstance()->assertNoLeak();
    }
#endif
}

void run() {
    // Init log with this thread.
    WinDSPLog::init();

    // Load config file
    const string configPath = OS::getExeDirPath() + getConfigFileName();
    pConfig = make_shared<Config>(configPath);

    // Log title to console.
    logTitle();

    // Update start with OS.
    updateStartWithOS();

    // Get capture and render devices
    const string captureDeviceName = pConfig->getCaptureDeviceName();
    const string renderDeviceName = pConfig->getRenderDeviceName();

    /*
     * Create and initalize devices and validate device settings
     */

    unique_ptr<AudioDevice> pCaptureDevice, pRenderDevice;

    pCaptureDevice = AudioDevice::initDevice(captureDeviceName);
    pCaptureDevice->initCaptureService();
    const WAVEFORMATEX* const pCaptureFormat = pCaptureDevice->getFormat();
    // Sample buffers must contains a 32bit float. All code depends on it.
    if (pCaptureFormat->wBitsPerSample != 32) {
        throw Error("Bit depth doesnt match float(32), Found(%d)", pCaptureFormat->wBitsPerSample);
    }

    uint32_t renderNumChannels;

    // ASIO render device.
    if (pConfig->useAsioRenderDevice()) {
        renderNumChannels = pConfig->getAsioNumChannels() > 0
            ? pConfig->getAsioNumChannels()
            : pCaptureDevice->getFormat()->nChannels;
        AsioDevice::initRenderService(
            renderDeviceName, pCaptureFormat->nSamplesPerSec,
            pConfig->getAsioBufferSize(), renderNumChannels, pConfig->inDebug());
    }
    // WASAPI render device.
    else {
        pRenderDevice = AudioDevice::initDevice(renderDeviceName);
        pRenderDevice->initRenderService();
        const WAVEFORMATEX* const pRenderFormat = pRenderDevice->getFormat();
        renderNumChannels = pRenderFormat->nChannels;
        // The application have no resampler. Sample rate and bit depth must be a match.
        if (pCaptureFormat->nSamplesPerSec != pRenderFormat->nSamplesPerSec) {
            throw Error("Sample rate missmatch: Capture(%d), Render(%d)",
                pCaptureFormat->nSamplesPerSec, pRenderFormat->nSamplesPerSec);
        }
        if (pCaptureFormat->wBitsPerSample != pRenderFormat->wBitsPerSample) {
            throw Error("Bit depth missmatch: Capture(%d), Render(%d)",
                pCaptureFormat->wBitsPerSample, pRenderFormat->wBitsPerSample);
        }
        if (pCaptureFormat->wFormatTag != pRenderFormat->wFormatTag) {
            throw Error("Format tag missmatch: Capture(%d), Render(%d)",
                pCaptureFormat->wFormatTag, pRenderFormat->wFormatTag);
        }
    }

    // Read config and get I/O instances with filters 
    pConfig->init(pCaptureFormat->nSamplesPerSec, pCaptureFormat->nChannels, renderNumChannels);

    /*
     * Print data to the user.
     */

    LOG_INFO("----------------------------------------------");
    LOG_INFO("Starting DSP service @ %s", Date::toLocalDateTimeString().c_str());
    LOG_INFO("Capture : %s - WASAPI", captureDeviceName.c_str());
    if (pConfig->useAsioRenderDevice()) {
        LOG_INFO("Render  : %s - ASIO", renderDeviceName.c_str());
        if (pConfig->inDebug()) {
            LOG_INFO("ASIO buffer: %d(%.1fms)",
                AsioDevice::getBufferSize(),
                1000.0 * AsioDevice::getBufferSize() / pCaptureFormat->nSamplesPerSec
            );
        }
    }
    else {
        LOG_INFO("Render  : %s - WASAPI", renderDeviceName.c_str());
    }
    if (pConfig->inDebug()) {
        LOG_INFO("Log file: %s", LOG_FILE);
    }
    if (pConfig->hasDescription()) {
        LOG_INFO("%s", pConfig->getDescription().c_str());
    }
    LOG_INFO("----------------------------------------------");
    LOG_NL();
    if (pConfig->inDebug()) {
        pConfig->printConfig();
    }

    // Show or hide window. Do this in late stage; irritating when winow is hidden/shown for every failed hardware init.
    Visibility::update(pConfig.get());

    // Start capturing data
    CaptureLoop captureLoop(pConfig, pCaptureDevice, pRenderDevice);
    captureLoop.run();
}

void main() {
    setTitle();

    // Very important for filter performance.
    OS::setPriorityHigh();

    size_t waiting = 0;
    for (;;) {
        try {
            // Run application
            run();
        }
        catch (const ConfigChangedException& e) {
            WinDSPLog::flush();
            if (!checkInput(e.getInput())) {
                LOG_INFO("Config file changed. Restarting...");
                LOG_NL();
            }
        }
        // Keep trying for the service to come back
        catch (const exception& e) {
            Visibility::show(true);
            WinDSPLog::flush();
            LOG_ERROR("ERROR: %s", e.what());

            // Wait before trying again.
            waiting = 20;
        }

        // Release old resources.
        clearData();

        // Wait for device to possible come back after reconfigure.
        while (waiting > 0) {
            --waiting;

            // Sleep not to busy wait all resources.
            Date::sleepMillis(100);

            // Check keyboard input. Needed if user want to change config during error state.
            checkInput(Keyboard::getDigit());
        }
    }
}