#include "AudioDevice.h"
#include "Functiondiscoverykeys_devpkey.h" //PKEY_Device_FriendlyName

#define SAFE_RELEASE(punk) if ((punk) != NULL) { (punk)->Release(); (punk) = NULL; }

IMMDeviceEnumerator *AudioDevice::_pEnumerator = nullptr;
bool AudioDevice::_initStatic = false;

std::vector<std::string> AudioDevice::getDeviceNames() {
	std::vector<std::string> result;
	initStatic();
	IMMDeviceCollection *pCollection;
	assert(_pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection));
	UINT count;
	assert(pCollection->GetCount(&count));
	for (ULONG i = 0; i < count; i++) {
		IMMDevice *pDevice;
		assert(pCollection->Item(i, &pDevice));
		result.push_back(getDeviceName(pDevice));
		SAFE_RELEASE(pDevice);
	}
	SAFE_RELEASE(pCollection);
	return result;
}

std::string AudioDevice::getDeviceName(IMMDevice *pDevice) {
	IPropertyStore *pProps = NULL;
	assert(pDevice->OpenPropertyStore(STGM_READ, &pProps));
	//Initialize container for property value.
	PROPVARIANT varName;
	PropVariantInit(&varName);
	//Get the endpoint's friendly-name property.
	assert(pProps->GetValue(PKEY_Device_FriendlyName, &varName));
	std::wstring tmp(varName.pwszVal);
	std::string result = std::string(tmp.begin(), tmp.end());
	PropVariantClear(&varName);
	SAFE_RELEASE(pProps);
	return result;
}

AudioDevice* AudioDevice::initDevice(const std::string &name) {
	initStatic();
	IMMDeviceCollection *pCollection;
	assert(_pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection));
	UINT count;
	AudioDevice* result = nullptr;
	assert(pCollection->GetCount(&count));
	for (ULONG i = 0; i < count; i++) {
		IMMDevice *pDevice;
		assert(pCollection->Item(i, &pDevice));
		if (name.compare(getDeviceName(pDevice)) == 0) {
			result = new AudioDevice(pDevice);
			break;
		}
		SAFE_RELEASE(pDevice);
	}
	SAFE_RELEASE(pCollection);
	if (!result) {
		throw Error("Can't find WASAPI endpoint '%s'", name.c_str());
	}
	return result;
}

void AudioDevice::initStatic() {
	if (!_initStatic) {
		assert(CoInitialize(nullptr));
		assert(CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&_pEnumerator));
		_initStatic = true;
	}
}

void AudioDevice::destroyStatic() {
	SAFE_RELEASE(_pEnumerator);
	_initStatic = false;
}

AudioDevice::AudioDevice() {
	initDefault();
	assert(_pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &_pDevice));
	init(_pDevice);
}

AudioDevice::AudioDevice(const std::string &id) : AudioDevice(std::wstring(id.begin(), id.end())) {
}

AudioDevice::AudioDevice(const std::wstring &id) {
	initDefault();
	assert(_pEnumerator->GetDevice(id.c_str(), &_pDevice));
	init(_pDevice);
}

AudioDevice::AudioDevice(IMMDevice *pDevice) {
	initDefault();
	init(pDevice);
}

AudioDevice::~AudioDevice() {
	//Stop service;
	_pAudioClient->Stop();
	//Free resources
	CoTaskMemFree(_pFormat);
	SAFE_RELEASE(_pDevice);
	SAFE_RELEASE(_pAudioClient);
	SAFE_RELEASE(_pCaptureClient);
	SAFE_RELEASE(_pRenderClient);
	SAFE_RELEASE(_pSimpleVolume);
}

void AudioDevice::initDefault() {
	initStatic();
	_pFormat = nullptr;
	_pDevice = nullptr;
	_pAudioClient = nullptr;
	_pCaptureClient = nullptr;
	_pRenderClient = nullptr;
	_pSimpleVolume = nullptr;
	_bufferSize = 0;
}

void AudioDevice::init(IMMDevice *pDevice) {
	_pDevice = pDevice;
	assert(_pDevice->Activate(__uuidof(IAudioClient3), CLSCTX_ALL, NULL, (void**)&_pAudioClient));
	assert(_pAudioClient->GetMixFormat(&_pFormat));
}

void AudioDevice::initCaptureService() {
	prepareService(true);
}

void AudioDevice::initRenderService() {
	prepareService(false);
}

void AudioDevice::startService() {
	//Start aduio service on device.
	assert(_pAudioClient->Start());
}

void AudioDevice::prepareService(const bool capture) {
	if (capture) {
		assert(_pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0, _pFormat, nullptr));
		assert(_pAudioClient->GetService(IID_PPV_ARGS(&_pCaptureClient)));
		assert(_pAudioClient->GetBufferSize(&_bufferSize));
	}
	else {
		//Create event handle
		_eventHandle = CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
		if (_eventHandle == NULL) {
			throw Error("WASAPI: Unable to create samples ready event %d", GetLastError());
		}
		assert(_pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 0, 0, _pFormat, nullptr));
		assert(_pAudioClient->SetEventHandle(_eventHandle));
		//assert(_pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 0, 0, _pFormat, nullptr));
		assert(_pAudioClient->GetService(IID_PPV_ARGS(&_pRenderClient)));
		WAVEFORMATEX *p;
		_pAudioClient->GetCurrentSharedModeEnginePeriod(&p, &_bufferSize);
	}
}

void AudioDevice::printInfo() const {
	REFERENCE_TIME engineTime, minTime;
	assert(_pAudioClient->GetDevicePeriod(&engineTime, &minTime));
	UINT32 default_, fundamental, min, max;
	assert(_pAudioClient->GetSharedModeEnginePeriod(_pFormat, &default_, &fundamental, &min, &max));
	printf("hw min = %f, engine = %f, buffer = %f\n", minTime / 10000.0, engineTime / 10000.0, 1000.0 * _bufferSize / _pFormat->nSamplesPerSec);
	printf("default = %d, fundamental = %d, min = %d, max = %d, current = %d, buffer = %d\n", default_, fundamental, min, max, _bufferSize, _bufferSize);
}

const std::string AudioDevice::getId() {
	if (!_id.size()) {
		LPWSTR id;
		assert(_pDevice->GetId(&id));
		std::wstring tmp(id);
		_id = std::string(tmp.begin(), tmp.end());
		CoTaskMemFree(id);
	}
	return _id;
}

const std::string AudioDevice::getName() {
	if (!_name.size()) {
		_name = getDeviceName(_pDevice);
	}
	return _name;
}

const WAVEFORMATEX* AudioDevice::getFormat() const {
	return _pFormat;
}

ISimpleAudioVolume* AudioDevice::getVolumeControl() {
	if (!_pSimpleVolume) {
		assert(_pAudioClient->GetService(__uuidof(ISimpleAudioVolume), (void**)&_pSimpleVolume));
	}
	return _pSimpleVolume;
}