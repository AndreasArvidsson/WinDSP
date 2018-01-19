#include "AudioDevice.h"
#include "Functiondiscoverykeys_devpkey.h"

#define SAFE_RELEASE(punk) if ((punk) != NULL) { (punk)->Release(); (punk) = NULL; }

IMMDeviceEnumerator *AudioDevice::_pEnumerator = nullptr;
bool AudioDevice::_initStatic = false;

std::vector<AudioDevice*> AudioDevice::getDevices() {
	initStatic();
	IMMDeviceCollection *pCollection;
	HRESULT hr = _pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection);
	assertHrResult(hr);
	UINT count;
	hr = pCollection->GetCount(&count);
	assertHrResult(hr);
	std::vector<AudioDevice*> devices;
	for (ULONG i = 0; i < count; i++) {
		IMMDevice *pDevice;
		hr = pCollection->Item(i, &pDevice);
		devices.push_back(new AudioDevice(pDevice));
		assertHrResult(hr);
	}
	SAFE_RELEASE(pCollection);
	return devices;
}

void AudioDevice::initStatic() {
	if (!_initStatic) {
		HRESULT hr = CoInitialize(nullptr);
		assertHrResult(hr);
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&_pEnumerator);
		assertHrResult(hr);
		_initStatic = true;
	}
}

void AudioDevice::destroyStatic() {
	SAFE_RELEASE(_pEnumerator);
	_initStatic = false;
}

AudioDevice::AudioDevice() {
	init();
	HRESULT hr = _pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &_pDevice);
	assertHrResult(hr);
	init(_pDevice);
}

AudioDevice::AudioDevice(const std::string &id) : AudioDevice(std::wstring(id.begin(), id.end())) {
}

AudioDevice::AudioDevice(const std::wstring &id) {
	init();
	HRESULT hr = _pEnumerator->GetDevice(id.c_str(), &_pDevice);
	assertHrResult(hr);
	init(_pDevice);
}

AudioDevice::AudioDevice(IMMDevice *pDevice) {
	init(pDevice);
}

void AudioDevice::init() {
	initStatic();
	_pFormat = nullptr;
	_pDevice = nullptr;
	_pAudioClient = nullptr;
	_pCaptureClient = nullptr;
	_pRenderClient = nullptr;
	_pSimpleVolume = nullptr;
}

void AudioDevice::init(IMMDevice *pDevice) {
	_pDevice = pDevice;

	HRESULT hr = _pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&_pAudioClient);
	assertHrResult(hr);

	hr = _pAudioClient->GetMixFormat(&_pFormat);
	assertHrResult(hr);

	_initMode = false;
	_bufferFrameCount = -1;
}

void AudioDevice::initMode(const bool capture) {
	if (!_initMode) {
		HRESULT hr;
		if (capture) {
			hr = _pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0, _pFormat, 0);
		}
		else {
			hr = _pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 0, 0, _pFormat, 0);
		}
		assertHrResult(hr);

		//Get the size of the allocated buffer.
		hr = _pAudioClient->GetBufferSize(&_bufferFrameCount);
		assertHrResult(hr);

		_initMode = true;
	}
}

AudioDevice::~AudioDevice() {
	CoTaskMemFree(_pFormat);
	SAFE_RELEASE(_pDevice);
	SAFE_RELEASE(_pAudioClient);
	SAFE_RELEASE(_pCaptureClient);
	SAFE_RELEASE(_pRenderClient);
	SAFE_RELEASE(_pSimpleVolume);
}

const std::string AudioDevice::getId() {
	if (!_id.size()) {
		LPWSTR id;
		HRESULT hr = _pDevice->GetId(&id);
		assertHrResult(hr);
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
		assertHrResult(hr);

		//Initialize container for property value.
		PROPVARIANT varName;
		PropVariantInit(&varName);

		//Get the endpoint's friendly-name property.
		hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
		assertHrResult(hr);

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
	assertHrResult(hr);
	return _bufferFrameCount - numFramesPadding;
}

IAudioCaptureClient* AudioDevice::getCaptureClient() {
	if (!_pCaptureClient) {
		initMode(true);
		HRESULT hr = _pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&_pCaptureClient);
		assertHrResult(hr);
	}
	return _pCaptureClient;
}

IAudioRenderClient* AudioDevice::getRenderClient() {
	if (!_pRenderClient) {
		initMode(false);
		HRESULT hr = _pAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&_pRenderClient);
		assertHrResult(hr);
	}
	return _pRenderClient;
}

ISimpleAudioVolume* AudioDevice::getVolume() {
	if (!_pSimpleVolume) {
		HRESULT hr = _pAudioClient->GetService(__uuidof(ISimpleAudioVolume), (void**)&_pSimpleVolume);
		assertHrResult(hr);
	}
	return _pSimpleVolume;
}

void AudioDevice::start() {
	HRESULT hr = _pAudioClient->Start();
	assertHrResult(hr);
}

void AudioDevice::stop() {
	HRESULT hr = _pAudioClient->Stop();
	assertHrResult(hr);
}

void AudioDevice::printInfo() {
	printf("%s '%s'\n", getName().c_str(), getId().c_str());
}



//_pFormat->wFormatTag = WAVE_FORMAT_PCM;
//_pFormat->nChannels = 2;
//_pFormat->wBitsPerSample = 16;
//_pFormat->nSamplesPerSec = 44100;
//_pFormat->nBlockAlign = _pFormat->nChannels * _pFormat->wBitsPerSample / 8;
//_pFormat->nAvgBytesPerSec = _pFormat->nSamplesPerSec * _pFormat->nBlockAlign;
//_pFormat->cbSize = 0;
//hr = _pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, _pFormat, 0);
//AUDCLNT_E_UNSUPPORTED_FORMAT
//hr = _pAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, 0, 0, 0, _pFormat, 0);