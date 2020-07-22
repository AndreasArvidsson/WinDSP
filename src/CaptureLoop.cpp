#include "CaptureLoop.h"
#include "WinDSPLog.h"
#include "Keyboard.h"
#include "Date.h"
#include "Convert.h"
#include "ConfigChangedException.h"
#include "Config.h"
#include "TrayIcon.h"
#include "OS.h"
#include "Visibility.h"
#include "AudioDevice.h"
#include "AsioDevice.h"
#include "Input.h"
#include "Output.h"

//#define PERFORMANCE_LOG

#ifdef PERFORMANCE_LOG
#include "Stopwatch.h"
Stopwatch sw("Render", 5000);
#define swStart() sw.intervalStart()
#define swEnd() sw.intervalEnd()
#else
#define swStart() (void)0
#define swEnd() (void)0
#endif

CaptureLoop::CaptureLoop(const shared_ptr<Config> pConfig, unique_ptr<AudioDevice>& pCaptureDevice, unique_ptr<AudioDevice>& pRenderDevice) {
    _pConfig = pConfig;
    _pInputs = &pConfig->getInputs();
    _pOutputs = &pConfig->getOutputs();
    _pCaptureDevice = move(pCaptureDevice);
    _pRenderDevice = move(pRenderDevice);
    _run = false;

    //Initialize conditions
    Condition::init(_pInputs->size());
}

CaptureLoop::~CaptureLoop() {
    //Stop and wait for capture thread to finish.
    _run = false;
    _captureThread.join();
    Condition::destroy();
}

void CaptureLoop::run() {
    _run = true;

    //Start wasapi capture device.
    _pCaptureDevice->startService();

    if (_pConfig->useAsioRenderDevice()) {
        //Asio rendering is started in a new thread by the driver.
        AsioDevice::startService();
        //Start capturing in a new thread.
        _captureThread = thread(&CaptureLoop::_captureLoopAsio, this);
    }
    else {
        //Start wasapi render device.
        _pRenderDevice->startService();
        //Start capturing in a new thread.
        _captureThread = thread(&CaptureLoop::_captureLoopWasapi, this);
    }

    size_t count = 0;
    for (;;) {
        //Dont print in other than main thread due top performance/latency issues.
        WinDSPLog::flush();

        //Asio renderer operates in its on thread context and cant directly throw exceptions.
        AsioDevice::throwError();
        AudioDevice::throwError();

        //Check if config file has changed.
        _checkConfig();

        if (TrayIcon::isShown()) {
            //Check if tray icon is clicked.
            TrayIcon::handleQueue();
        }
        else {
            //If hide is enabled and the window is minimize hide it and show tray icon instead.
            if (_pConfig->hide() && OS::isWindowMinimized()) {
                Visibility::hide();
            }
        }

        //Dont do as often. Every 5second or so.
        if (count == 50) {
            count = 0;

            //Check if clipping has occured since last.
            _checkClippingChannels();

            //Update conditional routing requirements since last.
            _updateConditionalRouting();
        }
        ++count;

        //Short sleep just to not busy wait all resources.
        Date::sleepMillis(100);
    }
}

void CaptureLoop::_captureLoopAsio() {
    //Used to temporarily store sample(for all out channels) while they are being processed.
    double renderBlockBuffer[32];
    //The size of all sample frames for all channels with the same sample index/timestamp
    const size_t renderBlockSize = sizeof(double) * _pOutputs->size();
    UINT32 samplesAvailable;
    DWORD flags;
    float* pCaptureBuffer;
    double* pRenderBlockBuffer;
    bool silent = true;
    bool first = true;

    //Wait until ASIO service has started.
    while (!AsioDevice::isRunning());

    while (_run) {
        //Check for samples in capture buffer.
        assert(_pCaptureDevice->getNextPacketSize(&samplesAvailable));

        while (samplesAvailable) {
            //Get capture buffer pointer and number of available frames.
            assert(_pCaptureDevice->getCaptureBuffer(&pCaptureBuffer, &samplesAvailable, &flags));

            if (flags) {
                if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                    //Was playing audio before. Stop rendering and reset filter states.
                    if (!silent) {
                        silent = true;
                        _resetFilters();
                    }
                    assert(_pCaptureDevice->releaseCaptureBuffer(samplesAvailable));
                    break;
                }
                else if (!first && _pConfig->inDebug()) {
                    if (flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) {
                        LOG_DEBUG("AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY: %d", samplesAvailable);
                    }
                    if (flags & AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR) {
                        LOG_DEBUG("AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR: %d", samplesAvailable);
                    }
                }
            }

            //Was silent before.
            if (silent) {
                silent = false;
                //First frames in capture buffer are bad for some strange reason. Part of the Wasapi standard.
                //Need to flush buffer of bad data or we can get glitches.
                if (first) {
                    first = false;
                    assert(_pCaptureDevice->releaseCaptureBuffer(samplesAvailable));
                    _pCaptureDevice->flushCaptureBuffer();
                    break;
                }

                //Render silence to asio to create the buffers at once. If not the first audio will be crackling.
                for (UINT32 sampleIndex = 0; sampleIndex < samplesAvailable; ++sampleIndex) {
                    for (UINT32 i = 0; i < _pOutputs->size(); ++i) {
                        AsioDevice::addSample(0);
                    }
                }
            }

            swStart();

            //Iterate all capture frames
            for (UINT32 sampleIndex = 0; sampleIndex < samplesAvailable; ++sampleIndex) {
                //Set buffer default value to 0 so we can add/mix values to it later
                memset(renderBlockBuffer, 0, renderBlockSize);

                //Iterate inputs and route samples to outputs
                for (Input& input : *_pInputs) {
                    input.route(*pCaptureBuffer++, renderBlockBuffer);
                }

                //Iterate outputs and apply filters
                pRenderBlockBuffer = renderBlockBuffer - 1;
                for (Output& output : *_pOutputs) {
                    AsioDevice::addSample(output.process(*++pRenderBlockBuffer));
                }
            }

            swEnd();

            //Release capture buffer.
            assert(_pCaptureDevice->releaseCaptureBuffer(samplesAvailable));

            //Check for more samples in capture buffer.
            assert(_pCaptureDevice->getNextPacketSize(&samplesAvailable));
        }

        //No available samples. Short sleep just to not busy wait all resources.
        Date::sleepMilli();
    }
}

void CaptureLoop::_captureLoopWasapi() {
    //Used to temporarily store sample(for all out channels) while they are being processed.
    double renderBlockBuffer[8];
    //The size of all sample frames for all channels with the same sample index/timestamp
    const size_t renderBlockSize = sizeof(double) * _pOutputs->size();
    UINT32 samplesAvailable;
    DWORD flags;
    float* pCaptureBuffer, * pRenderBuffer;
    double* pRenderBlockBuffer;
    bool silent = true;
    bool first = true;

    while (_run) {
        //Check for samples in capture buffer.
        assert(_pCaptureDevice->getNextPacketSize(&samplesAvailable));

        while (samplesAvailable && _run) {
            //Get capture buffer pointer and number of available frames.
            assert(_pCaptureDevice->getCaptureBuffer(&pCaptureBuffer, &samplesAvailable, &flags));

            if (flags) {
                if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                    //Was playing audio before. Reset filter states.
                    if (!silent) {
                        silent = true;
                        _resetFilters();
                    }
                    assert(_pCaptureDevice->releaseCaptureBuffer(samplesAvailable));
                    break;
                }
                else if (!first && _pConfig->inDebug()) {
                    if (flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) {
                        LOG_DEBUG("AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY: %d", samplesAvailable);
                    }
                    if (flags & AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR) {
                        LOG_DEBUG("AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR: %d", samplesAvailable);
                    }
                }
            }

            //Was silent before.
            if (silent) {
                silent = false;
                //Need to flush render buffer or a glitch kind of sound can occour after silence.
                _pRenderDevice->flushRenderBuffer();
                //First frames in capture buffer are bad for some strange reason. Part of the Wasapi standard.
                //Need to flush buffer of bad data or we can get glitches.
                if (first) {
                    first = false;
                    assert(_pCaptureDevice->releaseCaptureBuffer(samplesAvailable));
                    _pCaptureDevice->flushCaptureBuffer();
                    break;
                }
            }

            //Must read entire capture buffer at once. Wait until render buffer has enough space available.
            while (samplesAvailable > _pRenderDevice->getBufferFrameCountAvailable() && _run);

            //Get render buffer
            assert(_pRenderDevice->getRenderBuffer(&pRenderBuffer, samplesAvailable));

            if (pRenderBuffer) {
                swStart();

                //Iterate all capture frames
                for (UINT32 sampleIndex = 0; sampleIndex < samplesAvailable; ++sampleIndex) {
                    //Set buffer default value to 0 so we can add/mix values to it later
                    memset(renderBlockBuffer, 0, renderBlockSize);

                    //Iterate inputs and route samples to outputs
                    for (Input& input : *_pInputs) {
                        input.route(*pCaptureBuffer++, renderBlockBuffer);
                    }

                    //Iterate outputs and apply filters
                    pRenderBlockBuffer = renderBlockBuffer - 1;
                    for (Output& output : *_pOutputs) {
                        *pRenderBuffer++ = (float)output.process(*++pRenderBlockBuffer);
                    }
                }

                swEnd();
            }

            //Release render buffer.
            assert(_pRenderDevice->releaseRenderBuffer(samplesAvailable));

            //Release capture buffer.
            assert(_pCaptureDevice->releaseCaptureBuffer(samplesAvailable));

            //Check for more samples in capture buffer.
            assert(_pCaptureDevice->getNextPacketSize(&samplesAvailable));
        }

        //No available samples. Short sleep just to not busy wait all resources.
        Date::sleepMilli();
    }
}

void CaptureLoop::_resetFilters() {
    //Reset i/o filter states.
    for (Input& input : *_pInputs) {
        input.reset();
    }
    for (const Output& output : *_pOutputs) {
        output.reset();
    }
}

void CaptureLoop::_checkConfig() {
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

void CaptureLoop::_checkClippingChannels() {
    for (Output& output : *_pOutputs) {
        const double clipping = output.resetClipping();
        if (clipping != 0.0) {
            LOG_WARN("WARNING: Output(%s) - Clipping detected: +%0.2f dBFS", Channels::toString(output.getChannel()).c_str(), Convert::levelToDb(clipping));
        }
    }
}

void CaptureLoop::_updateConditionalRouting() {
    if (_pConfig->useConditionalRouting()) {
        //Get current is playing status per channel.
        for (size_t i = 0; i < _pInputs->size(); ++i) {
            Condition::setIsChannelUsed(i, (*_pInputs)[i].resetIsPlaying());
        }
        //Update conditional routing.
        for (Input& input : *_pInputs) {
            input.evalConditions();
        }
    }
}

//For debug purposes only.
void CaptureLoop::_printUsedChannels() {
    for (size_t i = 0; i < _pInputs->size(); ++i) {
        const Channel channel = (*_pInputs)[i].getChannel();
        LOG_INFO("%s %d", Channels::toString(channel).c_str(), Condition::isChannelUsed(i));
    }
    LOG_NL();
}