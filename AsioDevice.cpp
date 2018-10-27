#include "AsioDevice.h"
#include "asiosys.h"
#include "ErrorMessages.h"
#include "asiodrivers.h"
//#include "asiosys.h"


//External references
extern AsioDrivers* asioDrivers;
bool loadAsioDriver(char *name);

ASIOBufferInfo* AsioDevice::pBufferInfos = nullptr;
ASIOChannelInfo* AsioDevice::pChannelInfos = nullptr;
long AsioDevice::numInputChannels = 0;
long AsioDevice::numOutputChannels = 0;
long AsioDevice::numChannels = 0;
long AsioDevice::asioVersion = 0;
long AsioDevice::driverVersion = 0;
long AsioDevice::minSize = 0;
long AsioDevice::maxSize = 0;
long AsioDevice::preferredSize = 0;
long AsioDevice::granularity = 0;
long AsioDevice::bufferSize = 0;
long AsioDevice::inputLatency = 0;
long AsioDevice::outputLatency = 0;
double AsioDevice::sampleRate = 0.0;
bool AsioDevice::outputReady = false;
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
	assertAsio(ASIOGetChannels(&numInputChannels, &numOutputChannels));
	assertAsio(ASIOGetBufferSize(&minSize, &maxSize, &preferredSize, &granularity));
	bufferSize = preferredSize;
	assertAsio(ASIOGetSampleRate(&sampleRate));
	assertAsio(ASIOGetLatencies(&inputLatency, &outputLatency));
	outputReady = ASIOOutputReady() == ASE_OK;
}

void AsioDevice::startRenderService(ASIOCallbacks *pCallbacks, const long nChannels) {
	numChannels = nChannels > 0 ? min(nChannels, numOutputChannels) : numOutputChannels;

	//Create buffer info per channel.
	pBufferInfos = new ASIOBufferInfo[numChannels];
	ASIOBufferInfo *buf = pBufferInfos;
	for (int i = 0; i < numChannels; ++i, ++buf) {
		buf->isInput = ASIOFalse;
		buf->channelNum = i;
		buf->buffers[0] = buf->buffers[1] = 0;
	}

	//Use default message callback if not given.
	if (!pCallbacks->asioMessage) {
		pCallbacks->asioMessage = &_asioMessage;
	}

	//Create buffers and connect callbacks.
	assertAsio(ASIOCreateBuffers(pBufferInfos, numChannels, bufferSize, pCallbacks));

	//Get channel infos.
	pChannelInfos = new ASIOChannelInfo[numChannels];
	for (int i = 0; i < numChannels; ++i) {
		pChannelInfos[i].channel = pBufferInfos[i].channelNum;
		pChannelInfos[i].isInput = pBufferInfos[i].isInput;
		assertAsio(ASIOGetChannelInfo(&pChannelInfos[i]));
		assertSampleType(pChannelInfos[i].type);
	}

	assertAsio(ASIOStart());
}

void AsioDevice::stopRenderService() {
	ASIOStop();
	ASIODisposeBuffers();
}

void AsioDevice::destroy() {
	ASIOExit();
	delete[] pBufferInfos;
	delete[] pChannelInfos;
	delete _pDriverName;
	delete asioDrivers;
	pBufferInfos = nullptr;
	pChannelInfos = nullptr;
	_pDriverName = nullptr;
	asioDrivers = nullptr;
}

void AsioDevice::renderSilence(const long bufferIndex) {
	for (size_t channelIndex = 0; channelIndex < numChannels; ++channelIndex) {
		memset((int*)AsioDevice::pBufferInfos[channelIndex].buffers[bufferIndex], 0, AsioDevice::bufferSize * sizeof(int));
	}
}

const std::string AsioDevice::getName() {
	return *_pDriverName;
}

void AsioDevice::printInfo() {
	printf("asioVersion: %d\n", asioVersion);
	printf("driverVersion: %d\n", driverVersion);
	printf("Name: %s\n", _pDriverName->c_str());
	printf("ASIOGetChannels (inputs: %d, outputs: %d) - numChannels: %d\n", numInputChannels, numOutputChannels, numChannels);
	printf("ASIOGetBufferSize (min: %d, max: %d, preferred: %d, granularity: %d)\n", minSize, maxSize, preferredSize, granularity);
	printf("ASIOGetSampleRate (sampleRate: %f)\n", sampleRate);
	printf("ASIOGetLatencies (input: %d, output: %d)\n", inputLatency, outputLatency);
	printf("ASIOOutputReady(); - %s\n", outputReady ? "Supported" : "Not supported");
	if (pChannelInfos) {
		for (int i = 0; i < numOutputChannels; ++i) {
			printf("ASIOGetChannelInfo(channel: %d, name: %s, group: %d, isActive: %d, isInput: %d, type: %d)\n",
				pChannelInfos[i].channel, pChannelInfos[i].name, pChannelInfos[i].channelGroup, pChannelInfos[i].isActive, pChannelInfos[i].isInput, pChannelInfos[i].type);
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
	asioVersion = driverInfo.asioVersion;
	driverVersion = driverInfo.driverVersion;
}

long AsioDevice::_asioMessage(const long selector, const long value, void* const message, double* const opt) {
	return 0L;
}
