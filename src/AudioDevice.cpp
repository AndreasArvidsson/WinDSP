#include "AudioDevice.h"
#include "Mmdeviceapi.h"
#include "Functiondiscoverykeys_devpkey.h" //PKEY_Device_FriendlyName
#include "WinDSPLog.h"
#include "Str.h"

#define SAFE_RELEASE(punk) if ((punk) != NULL) { (punk)->Release(); (punk) = NULL; }

using std::make_unique;

IMMDeviceEnumerator* AudioDevice::_pEnumerator = nullptr;
bool AudioDevice::_initStatic = false;
atomic<bool> AudioDevice::_throwError = false;
Error AudioDevice::_error;

void AudioDevice::throwError() {
    if (_throwError) {
        throw _error;
    }
}

vector<string> AudioDevice::getDeviceNames() {
    vector<string> result;
    initStatic();
    IMMDeviceCollection* pCollection;
    assert(_pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection));
    UINT count;
    assert(pCollection->GetCount(&count));
    IMMDevice* pDevice;
    for (ULONG i = 0; i < count; ++i) {
        assert(pCollection->Item(i, &pDevice));
        result.push_back(getDeviceName(pDevice));
        SAFE_RELEASE(pDevice);
    }
    SAFE_RELEASE(pCollection);
    return result;
}

string AudioDevice::getDeviceName(IMMDevice* pDevice) {
    IPropertyStore* pProps = NULL;
    assert(pDevice->OpenPropertyStore(STGM_READ, &pProps));
    //Initialize container for property value.
    PROPVARIANT varName;
    PropVariantInit(&varName);
    //Get the endpoint's friendly-name property.
    assert(pProps->GetValue(PKEY_Device_FriendlyName, &varName));
    wstring tmp(varName.pwszVal);
    string result = String::toString(tmp);
    PropVariantClear(&varName);
    SAFE_RELEASE(pProps);
    return result;
}

unique_ptr<AudioDevice> AudioDevice::initDevice(const string& name) {
    initStatic();
    IMMDeviceCollection* pCollection;
    assert(_pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection));
    UINT count;
    unique_ptr<AudioDevice> result;
    assert(pCollection->GetCount(&count));
    IMMDevice* pDevice;
    for (ULONG i = 0; i < count; ++i) {
        assert(pCollection->Item(i, &pDevice));
        if (name.compare(getDeviceName(pDevice)) == 0) {
            result = make_unique<AudioDevice>(pDevice);
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
    _initStatic = _throwError = false;
}

AudioDevice::AudioDevice() {
    initDefault();
    assert(_pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &_pDevice));
    init(_pDevice);
}

AudioDevice::AudioDevice(const string& id) : AudioDevice(wstring(id.begin(), id.end())) {
}

AudioDevice::AudioDevice(const wstring& id) {
    initDefault();
    assert(_pEnumerator->GetDevice(id.c_str(), &_pDevice));
    init(_pDevice);
}

AudioDevice::AudioDevice(IMMDevice* pDevice) {
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

void AudioDevice::init(IMMDevice* pDevice) {
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
    }
    else {
        //Create event handle
        _eventHandle = CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
        if (_eventHandle == NULL) {
            throw Error("WASAPI: Unable to create samples ready event %d", GetLastError());
        }

        assert(_pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 0, 0, _pFormat, nullptr));
        assert(_pAudioClient->SetEventHandle(_eventHandle));
        assert(_pAudioClient->GetService(IID_PPV_ARGS(&_pRenderClient)));
    }

    //Get the size of the allocated buffer.
    assert(_pAudioClient->GetBufferSize(&_bufferSize));

    WAVEFORMATEX* p; //Need this. Cant be null.
    _pAudioClient->GetCurrentSharedModeEnginePeriod(&p, &_engineBufferSize);
}

void AudioDevice::printInfo() const {
    REFERENCE_TIME engineTime, minTime;
    assert(_pAudioClient->GetDevicePeriod(&engineTime, &minTime));
    UINT32 default_, fundamental, min, max;
    assert(_pAudioClient->GetSharedModeEnginePeriod(_pFormat, &default_, &fundamental, &min, &max));
    LOG_INFO("hw min = %f, engine = %f, buffer = %f", minTime / 10000.0, engineTime / 10000.0, 1000.0 * _bufferSize / _pFormat->nSamplesPerSec);
    LOG_INFO("default = %d, fundamental = %d, min = %d, max = %d, current = %d, buffer = %d", default_, fundamental, min, max, _bufferSize, _bufferSize);
}

const string AudioDevice::getId() {
    if (!_id.size()) {
        LPWSTR id;
        assert(_pDevice->GetId(&id));
        wstring tmp(id);
        _id = String::toString(tmp);
        CoTaskMemFree(id);
    }
    return _id;
}

const string AudioDevice::getName() {
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
        assert(_pAudioClient->GetService(IID_PPV_ARGS(&_pSimpleVolume)));
    }
    return _pSimpleVolume;
}

const UINT32 AudioDevice::getBufferSize() const {
    return _bufferSize;
}

const UINT32 AudioDevice::getEngineBufferSize() const {
    return _engineBufferSize;
}

void AudioDevice::flushCaptureBuffer() const {
    UINT32 frameCount;
    BYTE* buffer;
    DWORD flags;
    assert(_pCaptureClient->GetNextPacketSize(&frameCount));
    while (frameCount != 0) {
        assert(_pCaptureClient->GetBuffer(&buffer, &frameCount, &flags, nullptr, nullptr));
        assert(_pCaptureClient->ReleaseBuffer(frameCount));
        assert(_pCaptureClient->GetNextPacketSize(&frameCount));
    }
}

void AudioDevice::flushRenderBuffer() const {
    BYTE* buffer;
    UINT32 frameCount = getBufferFrameCountAvailable();
    while (frameCount != 0) {
        assert(_pRenderClient->GetBuffer(frameCount, &buffer));
        assert(_pRenderClient->ReleaseBuffer(frameCount, AUDCLNT_BUFFERFLAGS_SILENT));
        frameCount = getBufferFrameCountAvailable();
    }
}

//Convert HRESULT to text explanation.
const string AudioDevice::hresult(const HRESULT hr) {
    switch (hr) {
    case AUDCLNT_E_NOT_INITIALIZED:
        return "The audio stream has not been successfully initialized.";
    case AUDCLNT_E_ALREADY_INITIALIZED:
        return "The IAudioClient object is already initialized.";
    case AUDCLNT_E_WRONG_ENDPOINT_TYPE:
        return "The AUDCLNT_STREAMFLAGS_LOOPBACK flag is set but the endpoint device is a capture device, not a rendering device.";
    case AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED:
        return "The requested buffer size is not aligned. This code can be returned for a render or a capture device if the caller specified AUDCLNT_SHAREMODE_EXCLUSIVE and the AUDCLNT_STREAMFLAGS_EVENTCALLBACK flags. The caller must call Initialize again with the aligned buffer size. For more information, see Remarks.";
    case AUDCLNT_E_BUFFER_SIZE_ERROR:
        return "Indicates that the buffer duration value requested by an exclusive-mode client is out of range. The requested duration value for pull mode must not be greater than 500 milliseconds; for push mode the duration value must not be greater than 2 seconds.";
    case AUDCLNT_E_CPUUSAGE_EXCEEDED:
        return "Indicates that the process-pass duration exceeded the maximum CPU usage. The audio engine keeps track of CPU usage by maintaining the number of times the process-pass duration exceeds the maximum CPU usage. The maximum CPU usage is calculated as a percent of the engine's periodicity. The percentage value is the system's CPU throttle value (within the range of 10% and 90%). If this value is not found, then the default value of 40% is used to calculate the maximum CPU usage.";
    case AUDCLNT_E_DEVICE_INVALIDATED:
        return "The audio endpoint device has been unplugged, or the audio hardware or associated hardware resources have been reconfigured, disabled, removed, or otherwise made unavailable for use.";
    case AUDCLNT_E_DEVICE_IN_USE:
        return "The endpoint device is already in use. Either the device is being used in exclusive mode, or the device is being used in shared mode and the caller asked to use the device in exclusive mode.";
    case AUDCLNT_E_ENDPOINT_CREATE_FAILED:
        return "The method failed to create the audio endpoint for the render or the capture device. This can occur if the audio endpoint device has been unplugged, or the audio hardware or associated hardware resources have been reconfigured, disabled, removed, or otherwise made unavailable for use.";
    case AUDCLNT_E_INVALID_DEVICE_PERIOD:
        return "Indicates that the device period requested by an exclusive-mode client is greater than 500 milliseconds.";
    case AUDCLNT_E_UNSUPPORTED_FORMAT:
        return "The audio engine (shared mode) or audio endpoint device (exclusive mode) does not support the specified format.";
    case AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED:
        return "The caller is requesting exclusive-mode use of the endpoint device, but the user has disabled exclusive-mode use of the device.";
    case AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL:
        return "The AUDCLNT_STREAMFLAGS_EVENTCALLBACK flag is set but parameters hnsBufferDuration and hnsPeriodicity are not equal.";
    case AUDCLNT_E_SERVICE_NOT_RUNNING:
        return "The Windows audio service is not running.";
    case E_POINTER:
        return "Parameter pFormat is NULL.";
    case E_INVALIDARG:
        return "Parameter pFormat points to an invalid format description; or the AUDCLNT_STREAMFLAGS_LOOPBACK flag is set but ShareMode is not equal to AUDCLNT_SHAREMODE_SHARED; or the AUDCLNT_STREAMFLAGS_CROSSPROCESS flag is set but ShareMode is equal to AUDCLNT_SHAREMODE_EXCLUSIVE. A prior call to SetClientProperties was made with an invalid category for audio / render streams.";
    case E_OUTOFMEMORY:
        return "Out of memory.";
    case AUDCLNT_E_OUT_OF_ORDER:
        return "A previous IAudioRenderClient::GetBuffer call is still in effect.";
    case AUDCLNT_E_BUFFER_ERROR:
        return "GetBuffer failed to retrieve a data buffer and *ppData points to NULL. For more information, see Remarks.";
    case AUDCLNT_E_BUFFER_TOO_LARGE:
        return "The NumFramesRequested value exceeds the available buffer space (buffer size minus padding size).";
    case AUDCLNT_E_BUFFER_OPERATION_PENDING:
        return "Buffer cannot be accessed because a stream reset is in progress.";
    case AUDCLNT_E_NOT_STOPPED:
        return "The audio stream was not stopped at the time of the Start call.";
    case AUDCLNT_E_INVALID_SIZE:
        return "The NumFramesWritten value exceeds the NumFramesRequested value specified in the previous IAudioRenderClient::GetBuffer call.";
    case AUDCLNT_E_THREAD_NOT_REGISTERED:
        return "The thread is not registered.";
    case AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED:
        return "The audio stream was not initialized for event-driven buffering.";
    case AUDCLNT_E_EXCLUSIVE_MODE_ONLY:
        return "Exclusive mode only.";
    case AUDCLNT_S_BUFFER_EMPTY:
        return "The call succeeded and *pNumFramesToRead is 0, indicating that no capture data is available to be read.";
    case AUDCLNT_E_EVENTHANDLE_NOT_SET:
        return "The audio stream is configured to use event-driven buffering, but the caller has not called IAudioClient::SetEventHandle to set the event handle on the stream.";
    case AUDCLNT_E_INCORRECT_BUFFER_SIZE:
        return "Indicates that the buffer has an incorrect size.";
    case AUDCLNT_E_INVALID_STREAM_FLAG:
        return "AUDCLNT_E_INVALID_STREAM_FLAG";
    case AUDCLNT_E_ENDPOINT_OFFLOAD_NOT_CAPABLE:
        return "AUDCLNT_E_ENDPOINT_OFFLOAD_NOT_CAPABLE";
    case AUDCLNT_E_OUT_OF_OFFLOAD_RESOURCES:
        return "AUDCLNT_E_OUT_OF_OFFLOAD_RESOURCES";
    case AUDCLNT_E_OFFLOAD_MODE_ONLY:
        return "AUDCLNT_E_OFFLOAD_MODE_ONLY";
    case AUDCLNT_E_NONOFFLOAD_MODE_ONLY:
        return "AUDCLNT_E_NONOFFLOAD_MODE_ONLY";
    case AUDCLNT_E_RESOURCES_INVALIDATED:
        return "AUDCLNT_E_RESOURCES_INVALIDATED";
    case AUDCLNT_E_RAW_MODE_UNSUPPORTED:
        return "AUDCLNT_E_RAW_MODE_UNSUPPORTED";
    case AUDCLNT_S_THREAD_ALREADY_REGISTERED:
        return "AUDCLNT_S_THREAD_ALREADY_REGISTERED";
    case AUDCLNT_S_POSITION_STALLED:
        return "AUDCLNT_S_POSITION_STALLED";
    case E_NOT_SET:
        return "The audio endpoint device was not found.";
    default:
        return "Unknown error";
    }
}