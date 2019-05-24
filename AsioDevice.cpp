#include "AsioDevice.h"
#include "asiodrivers.h"
#include "Log.h"
#include "Error.h"
#include "Date.h"

#define MAX_INT32 2147483647.0

//External references
extern AsioDrivers* asioDrivers;
bool loadAsioDriver(char *name);

ASIOBufferInfo* AsioDevice::_pBufferInfos = nullptr;
ASIOChannelInfo* AsioDevice::_pChannelInfos = nullptr;
long AsioDevice::_numInputChannels = 0;
long AsioDevice::_numOutputChannels = 0;
long AsioDevice::_numChannels = 0;
long AsioDevice::_asioVersion = 0;
long AsioDevice::_driverVersion = 0;
long AsioDevice::_minSize = 0;
long AsioDevice::_maxSize = 0;
long AsioDevice::_preferredSize = 0;
long AsioDevice::_granularity = 0;
long AsioDevice::_bufferSize = 0;
long AsioDevice::_inputLatency = 0;
long AsioDevice::_outputLatency = 0;
double AsioDevice::_sampleRate = 0.0;
bool AsioDevice::_outputReady = false;
std::atomic<bool> AsioDevice::_run(false);
std::string *AsioDevice::_pDriverName = nullptr;
std::vector<double*> *AsioDevice::_pBuffers;
std::vector<double*> *AsioDevice::_pUnusedBuffers;
std::mutex AsioDevice::_buffersMutex;
std::mutex AsioDevice::_unusedBuffersMutex;
ASIOCallbacks AsioDevice::_callbacks{ 0 };

std::vector<std::string> AsioDevice::getDeviceNames() {
    std::vector<std::string> result;
    AsioDrivers asioDrivers;
    const int max = 128;
    char **driverNames = new char*[max];
    for (int i = 0; i < max; ++i) {
        driverNames[i] = new char[32];
    }
    const long numberOfAvailableDrivers = asioDrivers.getDriverNames(driverNames, max);
    for (int i = 0; i < numberOfAvailableDrivers; ++i) {
        result.push_back(driverNames[i]);
    }
    for (int i = 0; i < max; ++i) {
        delete[] driverNames[i];
    }
    delete[] driverNames;
    return result;
}

void AsioDevice::init(const std::string &dName, const long bufferSize, const long numChannels) {
    _loadDriver(dName);
    _assertAsio(ASIOGetChannels(&_numInputChannels, &_numOutputChannels));
    _assertAsio(ASIOGetBufferSize(&_minSize, &_maxSize, &_preferredSize, &_granularity));
    _assertAsio(ASIOGetSampleRate((ASIOSampleRate*)&_sampleRate));
    _outputReady = ASIOOutputReady() == ASE_OK;
    _bufferSize = bufferSize > 0 ? bufferSize : _preferredSize;
    _numChannels = numChannels > 0 ? min(numChannels, _numOutputChannels) : _numOutputChannels;
    _callbacks.asioMessage = &_asioMessage;
    _callbacks.bufferSwitch = &_bufferSwitch;

    _pBuffers = new std::vector<double*>;
    _pUnusedBuffers = new std::vector<double*>;

    //Create buffer info per channel.
    _pBufferInfos = new ASIOBufferInfo[_numChannels];
    for (int i = 0; i < _numChannels; ++i) {
        _pBufferInfos[i].channelNum = i;
        _pBufferInfos[i].buffers[0] = _pBufferInfos[i].buffers[1] = nullptr;
        _pBufferInfos[i].isInput = ASIOFalse;
    }

    //Get channel infos.
    _pChannelInfos = new ASIOChannelInfo[_numChannels];
    for (int i = 0; i < _numChannels; ++i) {
        _pChannelInfos[i].channel = _pBufferInfos[i].channelNum;
        _pChannelInfos[i].isInput = _pBufferInfos[i].isInput;
        _assertAsio(ASIOGetChannelInfo(&_pChannelInfos[i]));
        _assertSampleType(_pChannelInfos[i].type);
    }
}

void AsioDevice::startRenderService() {
    //Create buffers and connect callbacks.
    _assertAsio(ASIOCreateBuffers(_pBufferInfos, _numChannels, _bufferSize, &_callbacks));
    //Latencies are dependent on the used buffer size so have to be fetched after create buffers.
    _assertAsio(ASIOGetLatencies(&_inputLatency, &_outputLatency));
    //Start service.
    _assertAsio(ASIOStart());
}

void AsioDevice::stopRenderService() {
    _assertAsio(ASIOStop());
    _assertAsio(ASIODisposeBuffers());
    _run = false;
    //Move all buffers to unused list.
    _buffersMutex.lock();
    _unusedBuffersMutex.lock();
    while (_pBuffers->size() > 0) {
        _pUnusedBuffers->push_back(_pBuffers->back());
        _pBuffers->pop_back();
    }
    _buffersMutex.unlock();
    _unusedBuffersMutex.unlock();
}

void AsioDevice::destroy() {
    _assertAsio(ASIOExit());
    for (double *pBuffer : *_pUnusedBuffers) {
        delete[] pBuffer;
    }
    delete _pBuffers;
    delete _pUnusedBuffers;
    delete[] _pBufferInfos;
    delete[] _pChannelInfos;
    delete _pDriverName;
    delete asioDrivers;
    _pBuffers = nullptr;
    _pUnusedBuffers = nullptr;
    _pBufferInfos = nullptr;
    _pChannelInfos = nullptr;
    _pDriverName = nullptr;
    asioDrivers = nullptr;
}

const std::string AsioDevice::getName() {
    return *_pDriverName;
}

const long AsioDevice::getSampleRate() {
    return (long)_sampleRate;
}

const long AsioDevice::getNumOutputChannels() {
    return _numOutputChannels;
}

const long AsioDevice::getNumChannels() {
    return _numChannels;
}

const long AsioDevice::getBufferSize() {
    return _bufferSize;
}

double* AsioDevice::getWriteBuffer() {
    //If service is not running dont return a buffer.
    if (!_run) {
        return nullptr;
    }

    _unusedBuffersMutex.lock();
    //Check for available buffers.
    if (_pUnusedBuffers->size() > 0) {
        double* pBuffer = _pUnusedBuffers->back();
        _pUnusedBuffers->pop_back();
        _unusedBuffersMutex.unlock();
        return pBuffer;
    }
    _unusedBuffersMutex.unlock();
    //Create new buffer.
    LOG_INFO("Create buffer");
    return new double[_numChannels * _bufferSize];
}

void AsioDevice::addWriteBuffer(double * const pBuffer) {
    _buffersMutex.lock();
    _pBuffers->insert(_pBuffers->begin(), pBuffer);
    _buffersMutex.unlock();
}

double* AsioDevice::_getReadBuffer() {
    //Wait for an available buffer.
    for (;;) {
        _buffersMutex.lock();
        if (_pBuffers->size() > 0) {
            double *pBuffer = _pBuffers->back();
            _pBuffers->pop_back();
            _buffersMutex.unlock();
            return pBuffer;
        }
        _buffersMutex.unlock();
        Date::sleepMicros(1);
    }
}

void AsioDevice::_addReadBuffer(double * const pBuffer) {
    _unusedBuffersMutex.lock();
    _pUnusedBuffers->push_back(pBuffer);
    _unusedBuffersMutex.unlock();
}

void AsioDevice::printInfo() {
    LOG_INFO("asioVersion: %d\n", _asioVersion);
    LOG_INFO("driverVersion: %d\n", _driverVersion);
    LOG_INFO("Name: %s\n", _pDriverName->c_str());
    LOG_INFO("ASIOGetChannels (inputs: %d, outputs: %d) - numChannels: %d\n", _numInputChannels, _numOutputChannels, _numChannels);
    LOG_INFO("ASIOGetBufferSize (min: %d, max: %d, preferred: %d, granularity: %d)\n", _minSize, _maxSize, _preferredSize, _granularity);
    LOG_INFO("ASIOGetSampleRate (sampleRate: %d)\n", (int)_sampleRate);
    LOG_INFO("ASIOGetLatencies (input: %d, output: %d)\n", _inputLatency, _outputLatency);
    LOG_INFO("ASIOOutputReady(); - %s\n", _outputReady ? "Supported" : "Not supported");
    if (_pChannelInfos) {
        for (int i = 0; i < _numOutputChannels; ++i) {
            const ASIOChannelInfo &c = _pChannelInfos[i];
            LOG_INFO("ASIOGetChannelInfo(channel: %d, name: %s, group: %d, isActive: %d, isInput: %d, type: %d)\n", c.channel, c.name, c.channelGroup, c.isActive, c.isInput, c.type);
        }
    }
}

void AsioDevice::_loadDriver(const std::string &dName) {
    //Load the driver.
    if (!loadAsioDriver((char*)dName.c_str())) {
        std::string msg = "Failed to load ASIO driver ";
        msg += dName.c_str();
        throw new std::exception(msg.c_str());
    }
    //Initialize the driver.
    ASIODriverInfo driverInfo = { 0 };
    _assertAsio(ASIOInit(&driverInfo));
    _pDriverName = new std::string();
    *_pDriverName = driverInfo.name;
    _asioVersion = driverInfo.asioVersion;
    _driverVersion = driverInfo.driverVersion;
}

long AsioDevice::_asioMessage(const long selector, const long value, void * const message, double * const opt) {
    switch (selector) {
    case kAsioSelectorSupported:
        switch (value) {
        case kAsioResetRequest:
        case kAsioResyncRequest:
            return 1L;
        }
        break;
        //Ask the host application for its ASIO implementation. Host ASIO implementation version, 2 or higher 
    case kAsioEngineVersion:
        return 2L;
        //Requests a driver reset. Return value is always 1L.
    case kAsioResetRequest:
        LOG_ERROR("ASIO hardware is not available or has been reset.");
        return 1L;
        //The driver went out of sync, such that the timestamp is no longer valid.This is a request to re -start the engine and slave devices(sequencer). 1L if request is accepted or 0 otherwise.
    case kAsioResyncRequest:
        LOG_ERROR("ASIO driver went out of sync.");
        return 1L;
    }
    return 0L;
}

void AsioDevice::_bufferSwitch(const long asioBufferIndex, const ASIOBool) {
    //Signal first time that the render service is running.
    if (!_run) {
        _run = true;
        return;
    }

    double * const pReadBuffer = _getReadBuffer();

    for (size_t channelIndex = 0; channelIndex < _numChannels; ++channelIndex) {
        int *pRenderBuffer = (int*)_pBufferInfos[channelIndex].buffers[asioBufferIndex];
        for (size_t sampleIndex = 0; sampleIndex < _bufferSize; ++sampleIndex) {
            pRenderBuffer[sampleIndex] = (int)(MAX_INT32 * pReadBuffer[sampleIndex * _numChannels + channelIndex]);
        }
    }

    _addReadBuffer(pReadBuffer);

    //If available this can reduce latency.
    if (_outputReady) {
        _assertAsio(ASIOOutputReady());
    }
}

//Convert ASIOError to text explanation.
const std::string AsioDevice::_asioResult(const ASIOError error) {
    switch (error) {
    case ASE_OK:                return "This value will be returned whenever the call succeeded";
    case ASE_SUCCESS:           return "Unique success return value for ASIOFuture calls";
    case ASE_NotPresent:        return "Hardware input or output is not present or available";
    case ASE_HWMalfunction:     return "Hardware is malfunctioning (can be returned by any ASIO function)";
    case ASE_InvalidParameter:  return "Input parameter invalid";
    case ASE_InvalidMode:       return "Hardware is in a bad mode or used in a bad mode";
    case ASE_SPNotAdvancing:    return "Hardware is not running when sample position is inquired";
    case ASE_NoClock:           return "Sample clock or rate cannot be determined or is not present";
    case ASE_NoMemory:          return "Not enough memory for completing the request";
    default:                    return "Unknown error";
    }
}

const std::string AsioDevice::_asioSampleType(const ASIOSampleType type) {
    switch (type) {
    case ASIOSTInt16MSB:    return "Int16MSB";
    case ASIOSTInt24MSB:    return "Int24MSB";
    case ASIOSTInt32MSB:    return "Int32MSB";
    case ASIOSTFloat32MSB:  return "Float32MSB";
    case ASIOSTFloat64MSB:  return "Float64MSB";
    case ASIOSTInt32MSB16:  return "Int32MSB16";
    case ASIOSTInt32MSB18:  return "Int32MSB18";
    case ASIOSTInt32MSB20:  return "Int32MSB20";
    case ASIOSTInt32MSB24:  return "Int32MSB24";
    case ASIOSTInt16LSB:    return "Int16LSB";
    case ASIOSTInt24LSB:    return "Int24LSB";
    case ASIOSTInt32LSB:    return "Int32LSB";
    case ASIOSTFloat32LSB:  return "Float32LSB";
    case ASIOSTFloat64LSB:  return "Float64LSB";
    case ASIOSTInt32LSB16:  return "Int32LSB16";
    case ASIOSTInt32LSB18:  return "Int32LSB18";
    case ASIOSTInt32LSB20:  return "Int32LSB20";
    case ASIOSTInt32LSB24:  return "Int32LSB24";
    case ASIOSTDSDInt8LSB1: return "DSDInt8LSB1";
    case ASIOSTDSDInt8MSB1: return "DSDInt8MSB1";
    case ASIOSTDSDInt8NER8: return "DSDInt8NER8";
    default:                return std::to_string(type);
    }
}

void AsioDevice::_assertAsio(ASIOError error) {
    if (error != ASE_OK) {
        throw Error(_asioResult(error).c_str());
    }
}

void AsioDevice::_assertSampleType(ASIOSampleType type) {
    if (type != ASIOSTInt32LSB) {
        throw Error("Unnuported sample type: %s", _asioSampleType(type).c_str());
    }
}