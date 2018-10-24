#include "AsioDevice.h"
#include "ErrorMessages.h"
#include "asio/asiodrivers.h"

// some external references
extern AsioDrivers* asioDrivers;
bool loadAsioDriver(char *name);

AsioDrivers *AsioDevice::_pAsioDrivers = nullptr;
bool AsioDevice::_initStatic = false;

void AsioDevice::initStatic() {
	if (!_initStatic) {
		_pAsioDrivers = new AsioDrivers();
		_initStatic = true;
	}
}

void AsioDevice::destroyStatic() {
	delete _pAsioDrivers;
	_initStatic = false;
	//TODO 3 arrays still not freed
}

std::vector<std::string> AsioDevice::getDeviceNames() {
	initStatic();
	int max = 128;
	char **driverNames = new char*[max];
	for (int i = 0; i < max; ++i) {
		driverNames[i] = new char[32];
	}
	std::vector<std::string> result;
	long numberOfAvailableDrivers = _pAsioDrivers->getDriverNames(driverNames, max);
	for (int i = 0; i < numberOfAvailableDrivers; ++i) {
		result.push_back(driverNames[i]);
	}
	for (int i = 0; i < max; ++i) {
		delete[] driverNames[i];
	}
	delete[] driverNames;
	return result;
}

AsioDevice::AsioDevice(const char* driverName, const long numChannels, const HWND windowHandle) {
	_numChannelsRequested = numChannels;
	initStatic();
	//Load the driver, this will setup all the necessary internal data structures.
	assertTrue(_pAsioDrivers->loadDriver((char*)driverName), "Failed to load driver");
	//Initialize the driver.
	ASIODriverInfo driverInfo = { 0 };
	driverInfo.sysRef = windowHandle;
	assertAsio(ASIOInit(&driverInfo));
	_asioVersion = driverInfo.asioVersion;
	_driverVersion = driverInfo.driverVersion;
	_name = driverInfo.name;
	//Get number of supported channels.
	assertAsio(ASIOGetChannels(&_numInputChannels, &_numOutputChannels));
	//Get buffer asio size values.
	assertAsio(ASIOGetBufferSize(&_minSize, &_maxSize, &_preferredSize, &_granularity));
	//Use this size for the buffers.
	_bufferSize = _preferredSize;
	//Get sample rate
	assertAsio(ASIOGetSampleRate(&_sampleRate));
	//Get asio latencies
	assertAsio(ASIOGetLatencies(&_inputLatency, &_outputLatency));
	//Check wether the driver requires the ASIOOutputReady() optimization. (can be used by the driver to reduce output latency by one block)
	_postOutput = ASIOOutputReady() == ASE_OK;
}

AsioDevice::~AsioDevice() {
	delete[] _pBufferInfos;
	delete[] _pChannelInfos;
}

void AsioDevice::printInfo() const {
	printf("asioVersion: %d\ndriverVersion: %d\nName: %s\n", _asioVersion, _driverVersion, _name.c_str());
	printf("ASIOGetChannels (inputs: %d, outputs: %d);\n", _numInputChannels, _numOutputChannels);
	printf("ASIOGetBufferSize (min: %d, max: %d, preferred: %d, granularity: %d);\n", _minSize, _maxSize, _preferredSize, _granularity);
	printf("ASIOGetSampleRate (sampleRate: %f);\n", _sampleRate);
	printf("ASIOGetLatencies (input: %d, output: %d);\n", _inputLatency, _outputLatency);
	printf("ASIOOutputReady(); - %s\n", _postOutput ? "Supported" : "Not supported");
}

long asioMessageCallback(long selector, long value, void* message, double* opt) {
	switch (selector) {
	case kAsioSelectorSupported:
		switch (value) {
		case kAsioResetRequest:
		case kAsioEngineVersion:
		case kAsioResyncRequest:
		case kAsioLatenciesChanged:
			// the following three were added for ASIO 2.0, you don't necessarily have to support them
		case kAsioSupportsTimeInfo:
		case kAsioSupportsTimeCode:
		case kAsioSupportsInputMonitor:
			return 1L;
		}
		break;
	case kAsioResetRequest:
		// defer the task and perform the reset of the driver during the next "safe" situation
		// You cannot reset the driver right now, as this code is called from the driver.
		// Reset the driver is done by completely destruct is. I.e. ASIOStop(), ASIODisposeBuffers(), Destruction
		// Afterwards you initialize the driver again.
		//asioDriverInfo.stopped;  // In this sample the processing will just stop
		return 1L;
	case kAsioResyncRequest:
		// This informs the application, that the driver encountered some non fatal data loss.
		// It is used for synchronization purposes of different media.
		// Added mainly to work around the Win16Mutex problems in Windows 95/98 with the
		// Windows Multimedia system, which could loose data because the Mutex was hold too long
		// by another thread.
		// However a driver can issue it in other situations, too.
		return 1L;
	case kAsioLatenciesChanged:
		// This will inform the host application that the drivers were latencies changed.
		// Beware, it this does not mean that the buffer sizes have changed!
		// You might need to update internal delay data.
		return 1L;
	case kAsioEngineVersion:
		// return the supported ASIO version of the host application
		// If a host applications does not implement this selector, ASIO 1.0 is assumed
		// by the driver
		return 2L;
	case kAsioSupportsTimeInfo:
		// informs the driver wether the asioCallbacks.bufferSwitchTimeInfo() callback
		// is supported.
		// For compatibility with ASIO 1.0 drivers the host application should always support
		// the "old" bufferSwitch method, too.
		return 1L;
	case kAsioSupportsTimeCode:
		// informs the driver wether application is interested in time code info.
		// If an application does not need to know about time code, the driver has less work
		// to do.
		return 0L;
	}
	return 0L;
}

void AsioDevice::startRenderService(ASIOCallbacks *pCallbacks) {
	_numChannels = _numChannelsRequested > 0 ? min(_numChannelsRequested, _numOutputChannels) : _numOutputChannels;
	
	//Create buffer info per channel.
	_pBufferInfos = new ASIOBufferInfo[_numChannels];
	ASIOBufferInfo *buf = _pBufferInfos;
	for (int i = 0; i < _numChannels; ++i, ++buf) {
		buf->isInput = ASIOFalse;
		buf->channelNum = i;
		buf->buffers[0] = buf->buffers[1] = 0;
	}

	ASIOCallbacks callbacks{0};
	//Use default if not given.
	callbacks.asioMessage = pCallbacks->asioMessage ? pCallbacks->asioMessage : &asioMessageCallback;
	callbacks.bufferSwitch = pCallbacks->bufferSwitch;
	callbacks.bufferSwitchTimeInfo = pCallbacks->bufferSwitchTimeInfo;
	callbacks.sampleRateDidChange = pCallbacks->sampleRateDidChange;

	//Create buffers and connect callbacks.
	assertAsio(ASIOCreateBuffers(_pBufferInfos, _numChannels, _bufferSize, &callbacks));

	//Get channel infos.
	_pChannelInfos = new ASIOChannelInfo[_numChannels];
	for (int i = 0; i < _numChannels; ++i) {
		_pChannelInfos[i].channel = _pBufferInfos[i].channelNum;
		_pChannelInfos[i].isInput = _pBufferInfos[i].isInput;
		assertAsio(ASIOGetChannelInfo(&_pChannelInfos[i]));
		assertSampleType(_pChannelInfos[i].type);
	}

	//Start service.
	assertAsio(ASIOStart());
}

void AsioDevice::stopRenderService() {
	ASIOStop();
	ASIODisposeBuffers();
	//ASIOExit();
}

ASIOBufferInfo* AsioDevice::getBufferInfos() {
	return _pBufferInfos;
}

ASIOChannelInfo* AsioDevice::getChannelsInfos() {
	return _pChannelInfos;
}

const long AsioDevice::getBufferSize() {
	return _bufferSize;
}

const bool AsioDevice::getPostOutput() {
	return _postOutput;
}