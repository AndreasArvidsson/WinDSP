#include "AsioDevice.h"
#include <atomic>
#include <deque>
#include "asiodrivers.h"
#include "WinDSPLog.h"
#include "Error.h"
#include "SpinLock.h"
#include "Config.h"

#define MAX_INT32 2147483647.0

//External references
extern AsioDrivers* asioDrivers;
bool loadAsioDriver(char *name);

//Private variables
ASIOCallbacks _callbacks{ 0 };
ASIOBufferInfo* _pBufferInfos = nullptr;
ASIOChannelInfo* _pChannelInfos = nullptr;
std::string *_pDriverName = nullptr;
std::deque<double*> *_pUsedBuffers;
std::vector<double*> *_pUnusedBuffers;
double *_pCurrentWriteBuffer = nullptr;
const Config *_pConfig = nullptr;
std::atomic<bool> _running = false;
std::atomic<bool> _throwError = false;
bool _outputReady;
long _minSize, _maxSize, _preferredSize, _granularity, _bufferSize, _bufferByteSize, _numBuffers;
long _numInputChannels, _numOutputChannels, _asioVersion, _driverVersion, _inputLatency, _outputLatency;
long _numChannels, _currentWriteBufferCapacity, _currentWriteBufferSize;
double _sampleRate;
Error _error;
SpinLock _usedBuffersLock, _unusedBuffersLock;

std::vector<std::string> AsioDevice::getDeviceNames() {
    std::vector<std::string> result;
    AsioDrivers drivers;
    const int max = 128;
    char **driverNames = new char*[max];
    for (int i = 0; i < max; ++i) {
        driverNames[i] = new char[32];
    }
    const long numberOfAvailableDrivers = drivers.getDriverNames(driverNames, max);
    for (int i = 0; i < numberOfAvailableDrivers; ++i) {
        result.push_back(driverNames[i]);
    }
    for (int i = 0; i < max; ++i) {
        delete[] driverNames[i];
    }
    delete[] driverNames;
    return result;
}

void AsioDevice::initRenderService(const Config *pConfig, const std::string &dName, const long sampleRate, const long bufferSize, const long numChannels) {
    _pConfig = pConfig;
    _loadDriver(dName);
    _assertAsio(ASIOGetChannels(&_numInputChannels, &_numOutputChannels));
    _assertAsio(ASIOGetBufferSize(&_minSize, &_maxSize, &_preferredSize, &_granularity));
    _assertAsio(ASIOSetSampleRate(sampleRate));
    _assertAsio(ASIOGetSampleRate((ASIOSampleRate*)&_sampleRate));
    _outputReady = ASIOOutputReady() == ASE_OK;
    _bufferSize = bufferSize > 0 ? bufferSize : _preferredSize;
    _numChannels = numChannels > 0 ? min(numChannels, _numOutputChannels) : _numOutputChannels;
    _bufferByteSize = _bufferSize * sizeof(int);
    _callbacks.asioMessage = &_asioMessage;
    _callbacks.bufferSwitch = &_bufferSwitch;
    _pUsedBuffers = new std::deque<double*>;
    _pUnusedBuffers = new std::vector<double*>;
    _pCurrentWriteBuffer = nullptr;
    _throwError = false;
    _asioVersion = _driverVersion = _inputLatency = _outputLatency = 0;
    _numBuffers = _currentWriteBufferCapacity = _currentWriteBufferSize = 0;

    //Create buffer info per channel.
    _pBufferInfos = new ASIOBufferInfo[_numChannels];
    for (size_t i = 0; i < _numChannels; ++i) {
        _pBufferInfos[i].channelNum = (long)i;
        _pBufferInfos[i].buffers[0] = _pBufferInfos[i].buffers[1] = nullptr;
        _pBufferInfos[i].isInput = ASIOFalse;
    }

    //Get channel infos.
    _pChannelInfos = new ASIOChannelInfo[_numChannels];
    for (size_t i = 0; i < _numChannels; ++i) {
        _pChannelInfos[i].channel = _pBufferInfos[i].channelNum;
        _pChannelInfos[i].isInput = _pBufferInfos[i].isInput;
        _assertAsio(ASIOGetChannelInfo(&_pChannelInfos[i]));
        _assertSampleType(_pChannelInfos[i].type);
    }
}

void AsioDevice::destroy() {
    _assertAsio(ASIOExit());
    if (_pUnusedBuffers) {
        for (double *pBuffer : *_pUnusedBuffers) {
            delete[] pBuffer;
        }
    }
    delete _pUsedBuffers;
    delete _pUnusedBuffers;
    delete[] _pCurrentWriteBuffer;
    delete[] _pBufferInfos;
    delete[] _pChannelInfos;
    delete _pDriverName;
    delete asioDrivers;
    _pUsedBuffers = nullptr;
    _pUnusedBuffers = nullptr;
    _pCurrentWriteBuffer = nullptr;
    _pBufferInfos = nullptr;
    _pChannelInfos = nullptr;
    _pDriverName = nullptr;
    asioDrivers = nullptr;
}

void AsioDevice::startService() {
    //Create buffer first time.
    if (!_pCurrentWriteBuffer) {
        _pCurrentWriteBuffer = _getWriteBuffer();
        _currentWriteBufferCapacity = _numChannels * _bufferSize - 1;
    }
    //Empty current write buffer. Compensate for ++var operation.
    _currentWriteBufferSize = -1;
    //Create buffers and connect callbacks.
    _assertAsio(ASIOCreateBuffers(_pBufferInfos, _numChannels, _bufferSize, &_callbacks));
    //Latencies are dependent on the used buffer size so have to be fetched after create buffers.
    _assertAsio(ASIOGetLatencies(&_inputLatency, &_outputLatency));
    //Start service.
    _assertAsio(ASIOStart());
}

void AsioDevice::stopService() {
    ASIOStop();
    ASIODisposeBuffers();
    _running = false;
    reset();
}

void AsioDevice::reset() {
    //Empty current write buffer. Compensate for ++var operation.
    _currentWriteBufferSize = -1;
    //Move all buffers to unused list.
    _usedBuffersLock.lock();
    _unusedBuffersLock.lock();
    if (_pUsedBuffers) {
        while (_pUsedBuffers->size() > 0) {
            _pUnusedBuffers->push_back(_pUsedBuffers->back());
            _pUsedBuffers->pop_back();
        }
    }
    _usedBuffersLock.unlock();
    _unusedBuffersLock.unlock();
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

const bool AsioDevice::isRunning() {
    return _running;
}

void AsioDevice::throwError() {
    if (_throwError) {
        throw _error;
    }
}

void AsioDevice::addSample(const double sample) {
    _pCurrentWriteBuffer[++_currentWriteBufferSize] = sample;
    if (_currentWriteBufferSize == _currentWriteBufferCapacity) {
        _releaseWriteBuffer(_pCurrentWriteBuffer);
        _pCurrentWriteBuffer = _getWriteBuffer();
        _currentWriteBufferSize = -1;
    }
}

void AsioDevice::printInfo() {
    LOG_INFO("asioVersion: %d", _asioVersion);
    LOG_INFO("driverVersion: %d", _driverVersion);
    LOG_INFO("Name: %s", _pDriverName->c_str());
    LOG_INFO("ASIOGetChannels (inputs: %d, outputs: %d) - numChannels: %d", _numInputChannels, _numOutputChannels, _numChannels);
    LOG_INFO("ASIOGetBufferSize (min: %d, max: %d, preferred: %d, granularity: %d)", _minSize, _maxSize, _preferredSize, _granularity);
    LOG_INFO("ASIOGetSampleRate (sampleRate: %d)", (int)_sampleRate);
    LOG_INFO("ASIOGetLatencies (input: %d, output: %d)", _inputLatency, _outputLatency);
    LOG_INFO("ASIOOutputReady(); - %s", _outputReady ? "Supported" : "Not supported");
    if (_pChannelInfos) {
        for (size_t i = 0; i < _numOutputChannels; ++i) {
            const ASIOChannelInfo &c = _pChannelInfos[i];
            LOG_INFO("ASIOGetChannelInfo(channel: %d, name: %s, group: %d, isActive: %d, isInput: %d, type: %d)", c.channel, c.name, c.channelGroup, c.isActive, c.isInput, c.type);
        }
    }
}

void AsioDevice::_bufferSwitch(const long asioBufferIndex, const ASIOBool) {
    if (!_running) {
        _running = true;
    }

    double * const pReadBuffer = _getReadBuffer();

    //Read buffer available. Send data to render buffer.
    if (pReadBuffer) {
        for (size_t channelIndex = 0; channelIndex < _numChannels; ++channelIndex) {
            int * const pRenderBuffer = (int*)_pBufferInfos[channelIndex].buffers[asioBufferIndex];
            for (size_t sampleIndex = 0; sampleIndex < _bufferSize; ++sampleIndex) {
                pRenderBuffer[sampleIndex] = (int)(MAX_INT32 * pReadBuffer[sampleIndex * _numChannels + channelIndex]);
            }
        }
        _releaseReadBuffer(pReadBuffer);
    }
    //No data available. Just render silence.
    else {
        _renderSilence(asioBufferIndex);
    }

    //If available this can reduce latency.
    if (_outputReady) {
        _assertAsio(ASIOOutputReady());
    }
}

long AsioDevice::_asioMessage(const long selector, const long value, void * const, double * const) {
    switch (selector) {
    case kAsioSelectorSupported:
        switch (value) {
        case kAsioResetRequest:
        case kAsioBufferSizeChange:
        case kAsioResyncRequest:
        case kAsioLatenciesChanged:
        case kAsioOverload:
            return 1L;
        }
        break;
    case kAsioEngineVersion:
        return 2L;
    case kAsioResetRequest:
        _error = Error("ASIO reset request");
        _throwError = true;
        return 1L;
    case kAsioBufferSizeChange:
        _error = Error("ASIO hardware is not available or has been reset");
        _throwError = true;
        return 1L;
    case kAsioResyncRequest:
        _error = Error("ASIO driver went out of sync");
        _throwError = true;
        return 1L;
    case kAsioLatenciesChanged:
        _error = Error("ASIO latencies changed");
        _throwError = true;
        return 1L;
        //Dont support time info. Use regular bufferSwitch().
    case kAsioSupportsTimeInfo:
        return 0L;
    case kAsioOverload:
        _error = Error("ASIO overload");
        _throwError = true;
        return 1L;
    }
    if (_pConfig->inDebug()) {
        LOG_DEBUG("asioMessage(%d, %d)", selector, value);
    }
    return 0L;
}

double * const AsioDevice::_getWriteBuffer() {
    //Check for available buffers.
    _unusedBuffersLock.lock();
    if (_pUnusedBuffers->size() > 0) {
        double * const pBuffer = _pUnusedBuffers->back();
        _pUnusedBuffers->pop_back();
        _unusedBuffersLock.unlock();
        return pBuffer;
    }
    _unusedBuffersLock.unlock();
 
    if (_pConfig->inDebug()) {
        ++_numBuffers;
        const double timeDelay = 1000.0 * _numBuffers * _bufferSize / _sampleRate;
        LOG_DEBUG("Create ASIO buffer: %d(%.1fms)", _numBuffers, timeDelay);
    }

    //Create new buffer.
    return new double[_numChannels * _bufferSize];
}

double * const AsioDevice::_getReadBuffer() {
    _usedBuffersLock.lock();
    if (_pUsedBuffers->size() > 0) {
        double * const pBuffer = _pUsedBuffers->front();
        _pUsedBuffers->pop_front();
        _usedBuffersLock.unlock();
        return pBuffer;
    }
    _usedBuffersLock.unlock();
    return nullptr;
}

void AsioDevice::_releaseWriteBuffer(double * const pBuffer) {
    _usedBuffersLock.lock();
    _pUsedBuffers->push_back(pBuffer);
    _usedBuffersLock.unlock();
}

void AsioDevice::_releaseReadBuffer(double * const pBuffer) {
    _unusedBuffersLock.lock();
    _pUnusedBuffers->push_back(pBuffer);
    _unusedBuffersLock.unlock();
}

void AsioDevice::_renderSilence(const long asioBufferIndex) {
    for (size_t channelIndex = 0; channelIndex < _numChannels; ++channelIndex) {
        memset((int*)_pBufferInfos[channelIndex].buffers[asioBufferIndex], 0, _bufferByteSize);
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

void AsioDevice::_assertAsio(const ASIOError error) {
    if (error != ASE_OK) {
        throw Error(_asioResult(error).c_str());
    }
}

void AsioDevice::_assertSampleType(const ASIOSampleType type) {
    if (type != ASIOSTInt32LSB) {
        throw Error("Unnuported sample type: %s", _asioSampleType(type).c_str());
    }
}