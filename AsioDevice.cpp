#include "AsioDevice.h"
#include "asiosys.h"
#include "ErrorMessages.h"
#include "asiodrivers.h"
#include "WinDSPLog.h"

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
std::string *AsioDevice::_pDriverName = nullptr;

std::vector<std::string> AsioDevice::getDeviceNames() {
	std::vector<std::string> result;
	AsioDrivers asioDrivers;
	const int max = 128;
	char **driverNames = new char*[max];
	for (int i = 0; i < max; ++i) {
		driverNames[i] = new char[32];
	}
	long numberOfAvailableDrivers = asioDrivers.getDriverNames(driverNames, max);
	for (int i = 0; i < numberOfAvailableDrivers; ++i) {
		result.push_back(driverNames[i]);
	}
	for (int i = 0; i < max; ++i) {
		delete[] driverNames[i];
	}
	delete[] driverNames;
	return result;
}

void AsioDevice::init(const std::string &dName, const HWND windowHandle) {
	_loadDriver(dName, windowHandle);
	assertAsio(ASIOGetChannels(&_numInputChannels, &_numOutputChannels));
	assertAsio(ASIOGetBufferSize(&_minSize, &_maxSize, &_preferredSize, &_granularity));
	assertAsio(ASIOGetSampleRate(&_sampleRate));
	assertAsio(ASIOGetLatencies(&_inputLatency, &_outputLatency));
	_outputReady = ASIOOutputReady() == ASE_OK;
}

void AsioDevice::startRenderService(ASIOCallbacks *pCallbacks, const long bSize, const long nChannels) {
	_bufferSize = bSize > 0 ? bSize : _preferredSize;
	_numChannels = nChannels > 0 ? min(nChannels, _numOutputChannels) : _numOutputChannels;

	//Create buffer info per channel.
	_pBufferInfos = new ASIOBufferInfo[_numChannels];
	for (int i = 0; i < _numChannels; ++i) {
		_pBufferInfos[i].channelNum = i;
		_pBufferInfos[i].buffers[0] = _pBufferInfos[i].buffers[1] = nullptr;
		_pBufferInfos[i].isInput = ASIOFalse;
	}

	//Use default message callback if not given.
	if (!pCallbacks->asioMessage) {
		pCallbacks->asioMessage = &_asioMessage;
	}

	//Create buffers and connect callbacks.
	assertAsio(ASIOCreateBuffers(_pBufferInfos, _numChannels, _bufferSize, pCallbacks));

	//Get channel infos.
	_pChannelInfos = new ASIOChannelInfo[_numChannels];
	for (int i = 0; i < _numChannels; ++i) {
		_pChannelInfos[i].channel = _pBufferInfos[i].channelNum;
		_pChannelInfos[i].isInput = _pBufferInfos[i].isInput;
		assertAsio(ASIOGetChannelInfo(&_pChannelInfos[i]));
		assertSampleType(_pChannelInfos[i].type);
	}

	assertAsio(ASIOStart());
}

void AsioDevice::stopRenderService() {
	ASIOStop();
	ASIODisposeBuffers();
}

void AsioDevice::destroy() {
	ASIOExit();
	delete[] _pBufferInfos;
	delete[] _pChannelInfos;
	delete _pDriverName;
	delete asioDrivers;
	_pBufferInfos = nullptr;
	_pChannelInfos = nullptr;
	_pDriverName = nullptr;
	asioDrivers = nullptr;
}

void AsioDevice::renderSilence(const long bufferIndex) {
	static size_t channelIndex;
	for (channelIndex = 0; channelIndex < _numChannels; ++channelIndex) {
		memset((int*)_pBufferInfos[channelIndex].buffers[bufferIndex], 0, _bufferSize * sizeof(int));
	}
}

void AsioDevice::outputReady() {
	//If available this can reduce letency.
	if (_outputReady) {
		assertAsio(ASIOOutputReady());
	}
}

int* AsioDevice::getBuffer(const size_t channelIndex, const long bufferIndex) {
	return (int*)_pBufferInfos[channelIndex].buffers[bufferIndex];
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

void AsioDevice::_loadDriver(const std::string &dName, const HWND windowHandle) {
	//Load the driver.
	if (!loadAsioDriver((char*)dName.c_str())) {
		throw Error("Failed to load ASIO driver '%s'", dName.c_str());
	}
	//Initialize the driver.
	ASIODriverInfo driverInfo = { 0 };
	driverInfo.sysRef = windowHandle;
	assertAsio(ASIOInit(&driverInfo));
	_pDriverName = new std::string();
	*_pDriverName = driverInfo.name;
	_asioVersion = driverInfo.asioVersion;
	_driverVersion = driverInfo.driverVersion;
}

long AsioDevice::_asioMessage(const long selector, const long value, void* const message, double* const opt) {
	return 0L;
}
