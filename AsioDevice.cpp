#include "AsioDevice.h"
#include "ErrorMessages.h"
#include "asio/asiodrivers.h"

ASIOBufferInfo* AsioDevice::pBufferInfos = nullptr;
ASIOChannelInfo* AsioDevice::pChannelInfos = nullptr;
AsioDrivers* AsioDevice::pAsioDrivers = nullptr;
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
std::string AsioDevice::driverName = "";

std::vector<std::string> AsioDevice::getDeviceNames() {
	__initStatic();
	int max = 128;
	char **driverNames = new char*[max];
	for (int i = 0; i < max; ++i) {
		driverNames[i] = new char[32];
	}
	std::vector<std::string> result;
	long numberOfAvailableDrivers = pAsioDrivers->getDriverNames(driverNames, max);
	for (int i = 0; i < numberOfAvailableDrivers; ++i) {
		result.push_back(driverNames[i]);
	}
	for (int i = 0; i < max; ++i) {
		delete[] driverNames[i];
	}
	delete[] driverNames;
	return result;
}

void AsioDevice::init(char* const dName, const long nChannels, const HWND windowHandle) {
	__initStatic();
	__loadDriver(dName, windowHandle);
	__loadNumChannels(nChannels);
	assertAsio(ASIOGetBufferSize(&minSize, &maxSize, &preferredSize, &granularity));
	bufferSize = preferredSize;
	assertAsio(ASIOGetSampleRate(&sampleRate));
	assertAsio(ASIOGetLatencies(&inputLatency, &outputLatency));
	outputReady = ASIOOutputReady() == ASE_OK;
}

void AsioDevice::startRenderService(ASIOCallbacks *pCallbacks) {
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
		pCallbacks->asioMessage = &__asioMessage;
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
	//ASIOExit();
}

void AsioDevice::destroyStatic() {
	delete pAsioDrivers;
	delete[] pBufferInfos;
	delete[] pChannelInfos;
	//TODO 1 pointer for driverName string stil exists
	//TODO pointer3 arrays still not freed in driver class
}

void AsioDevice::printInfo() {
	printf("asioVersion: %d\n", asioVersion);
	printf("driverVersion: %d\n", driverVersion);
	printf("Name: %s\n", driverName.c_str());
	printf("ASIOGetChannels (inputs: %d, outputs: %d) - numChannels: %d\n", numInputChannels, numOutputChannels, numChannels);
	printf("ASIOGetBufferSize (min: %d, max: %d, preferred: %d, granularity: %d)\n", minSize, maxSize, preferredSize, granularity);
	printf("ASIOGetSampleRate (sampleRate: %f)\n", sampleRate);
	printf("ASIOGetLatencies (input: %d, output: %d)\n", inputLatency, outputLatency);
	printf("ASIOOutputReady(); - %s\n", outputReady ? "Supported" : "Not supported");
	if (pChannelInfos) {
		for (int i = 0; i < numChannels; ++i) {
			printf("ASIOGetChannelInfo(channel: %d, name: %s, group: %d, isActive: %d, isInput: %d, type: %d)\n",
				pChannelInfos[i].channel, pChannelInfos[i].name, pChannelInfos[i].channelGroup, pChannelInfos[i].isActive, pChannelInfos[i].isInput, pChannelInfos[i].type);
		}
	}
}

void AsioDevice::__loadDriver(char* const dName, const HWND windowHandle) {
	//Load the driver.
	if (!pAsioDrivers->loadDriver(dName)) {
		throw Error("Failed to load ASIO driver '%s'", dName);
	}
	//Initialize the driver.
	ASIODriverInfo driverInfo = { 0 };
	driverInfo.sysRef = windowHandle;
	assertAsio(ASIOInit(&driverInfo));
	driverName = driverInfo.name;
	asioVersion = driverInfo.asioVersion;
	driverVersion = driverInfo.driverVersion;
}

void AsioDevice::__loadNumChannels(const long nChannels) {
	assertAsio(ASIOGetChannels(&numInputChannels, &numOutputChannels));
	//Only output device for now.
	numChannels = min(nChannels, numOutputChannels);
}

void AsioDevice::__initStatic() {
	if (!pAsioDrivers) {
		pAsioDrivers = new AsioDrivers();
	}
}

long AsioDevice::__asioMessage(const long selector, const long value, void* const message, double* const opt) {
	return 0L;
}
