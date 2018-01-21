#include "AudioDevice.h"
#include "Functiondiscoverykeys_devpkey.h" //PKEY_Device_FriendlyName

#define SAFE_RELEASE(punk) if ((punk) != NULL) { (punk)->Release(); (punk) = NULL; }

IMMDeviceEnumerator *AudioDevice::_pEnumerator = nullptr;
bool AudioDevice::_initStatic = false;

std::vector<AudioDevice*> AudioDevice::getDevices() {
	initStatic();
	IMMDeviceCollection *pCollection;
	HRESULT hr = _pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection);
	assertHresult(hr);
	UINT count;
	hr = pCollection->GetCount(&count);
	assertHresult(hr);
	std::vector<AudioDevice*> devices;
	for (ULONG i = 0; i < count; i++) {
		IMMDevice *pDevice;
		hr = pCollection->Item(i, &pDevice);
		devices.push_back(new AudioDevice(pDevice));
		assertHresult(hr);
	}
	SAFE_RELEASE(pCollection);
	return devices;
}

void AudioDevice::initStatic() {
	if (!_initStatic) {
		HRESULT hr = CoInitialize(nullptr);
		assertHresult(hr);
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&_pEnumerator);
		assertHresult(hr);
		_initStatic = true;
	}
}

void AudioDevice::destroyStatic() {
	SAFE_RELEASE(_pEnumerator);
	_initStatic = false;
}

AudioDevice::AudioDevice() {
	initDefault();
	HRESULT hr = _pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &_pDevice);
	assertHresult(hr);
	init(_pDevice);
}

AudioDevice::AudioDevice(const std::string &id) : AudioDevice(std::wstring(id.begin(), id.end())) {
}

AudioDevice::AudioDevice(const std::wstring &id) {
	initDefault();
	HRESULT hr = _pEnumerator->GetDevice(id.c_str(), &_pDevice);
	assertHresult(hr);
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
	HRESULT hr = _pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&_pAudioClient);
	assertHresult(hr);
	hr = _pAudioClient->GetMixFormat(&_pFormat);
	assertHresult(hr);
}

void AudioDevice::startCaptureService() {
	startService(true);
}

void AudioDevice::startRenderService() {
	startService(false);
}

void AudioDevice::startService(const bool capture) {
	HRESULT hr;
	if (capture) {
		hr = _pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0, _pFormat, 0);
		assertHresult(hr);
		hr = _pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&_pCaptureClient);
		assertHresult(hr);
	}
	else {
		hr = _pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 0, 0, _pFormat, 0);
		assertHresult(hr);
		hr = _pAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&_pRenderClient);
		assertHresult(hr);
	}

	//Get the size of the allocated buffer.
	hr = _pAudioClient->GetBufferSize(&_bufferFrameCount);
	assertHresult(hr);

	if (!capture) {
		//Get and release render buffer first time. Needed to not get audio glitches
		BYTE *pRenderBuffer;
		hr = _pRenderClient->GetBuffer(_bufferFrameCount, &pRenderBuffer);
		assertHresult(hr);
		hr = _pRenderClient->ReleaseBuffer(_bufferFrameCount, AUDCLNT_BUFFERFLAGS_SILENT);
		assertHresult(hr);
	}

	//Start aduio service on device.
	hr = _pAudioClient->Start();
	assertHresult(hr);
}

const std::string AudioDevice::getId() {
	if (!_id.size()) {
		LPWSTR id;
		HRESULT hr = _pDevice->GetId(&id);
		assertHresult(hr);
		std::wstring tmp(id);
		_id = std::string(tmp.begin(), tmp.end());
		CoTaskMemFree(id);
	}
	return _id;
}

const std::string AudioDevice::getName() {
	if (!_name.size()) {
		IPropertyStore *pProps = NULL;

		HRESULT hr = _pDevice->OpenPropertyStore(STGM_READ, &pProps);
		assertHresult(hr);

		//Initialize container for property value.
		PROPVARIANT varName;
		PropVariantInit(&varName);

		//Get the endpoint's friendly-name property.
		hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
		assertHresult(hr);

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

UINT32 AudioDevice::getBufferFrameCountAvailable() const {
	UINT32 numFramesPadding;
	HRESULT hr = _pAudioClient->GetCurrentPadding(&numFramesPadding);
	assertHresult(hr);
	return _bufferFrameCount - numFramesPadding;
}

ISimpleAudioVolume* AudioDevice::getVolumeControl() {
	if (!_pSimpleVolume) {
		HRESULT hr = _pAudioClient->GetService(__uuidof(ISimpleAudioVolume), (void**)&_pSimpleVolume);
		assertHresult(hr);
	}
	return _pSimpleVolume;
}