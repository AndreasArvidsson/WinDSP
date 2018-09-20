#include "AudioDevice.h"
#include "Functiondiscoverykeys_devpkey.h" //PKEY_Device_FriendlyName

#define SAFE_RELEASE(punk) if ((punk) != NULL) { (punk)->Release(); (punk) = NULL; }

IMMDeviceEnumerator *AudioDevice::_pEnumerator = nullptr;
bool AudioDevice::_initStatic = false;

std::vector<AudioDevice*> AudioDevice::getDevices() {
	initStatic();
	IMMDeviceCollection *pCollection;
	assert(_pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection));
	UINT count;
	assert(pCollection->GetCount(&count));
	std::vector<AudioDevice*> devices;
	for (ULONG i = 0; i < count; i++) {
		IMMDevice *pDevice;
		assert(pCollection->Item(i, &pDevice));
		devices.push_back(new AudioDevice(pDevice));
	}
	SAFE_RELEASE(pCollection);
	return devices;
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
	_bufferFrameCount = 0;
}

void AudioDevice::init(IMMDevice *pDevice) {
	_pDevice = pDevice;
	assert(_pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&_pAudioClient));
	assert(_pAudioClient->GetMixFormat(&_pFormat));
}

void AudioDevice::startCaptureService() {
	startService(true);
}

void AudioDevice::startRenderService() {
	startService(false);
}

void AudioDevice::startService(const bool capture) {
	if (capture) {
		assert(_pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0, _pFormat, nullptr));
		assert(_pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&_pCaptureClient));
	}
	else {
		assert(_pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 0, 0, _pFormat, nullptr));
		assert(_pAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&_pRenderClient));
	}

	//Get the size of the allocated buffer.
	assert(_pAudioClient->GetBufferSize(&_bufferFrameCount));

	//Start aduio service on device.
	assert(_pAudioClient->Start());
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
		IPropertyStore *pProps = NULL;

		assert(_pDevice->OpenPropertyStore(STGM_READ, &pProps));

		//Initialize container for property value.
		PROPVARIANT varName;
		PropVariantInit(&varName);

		//Get the endpoint's friendly-name property.
		assert(pProps->GetValue(PKEY_Device_FriendlyName, &varName));

		std::wstring tmp(varName.pwszVal);
		_name = std::string(tmp.begin(), tmp.end());

		PropVariantClear(&varName);
		SAFE_RELEASE(pProps)
	}
	return _name;
}

const WAVEFORMATEX* AudioDevice::getFormat() const {
	return _pFormat;
}

UINT32 AudioDevice::getBufferFrameCount() const {
	return _bufferFrameCount;
}

ISimpleAudioVolume* AudioDevice::getVolumeControl() {
	if (!_pSimpleVolume) {
		assert(_pAudioClient->GetService(__uuidof(ISimpleAudioVolume), (void**)&_pSimpleVolume));
	}
	return _pSimpleVolume;
}