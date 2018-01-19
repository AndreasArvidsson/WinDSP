#pragma once
#include <vector>
#include "Mmdeviceapi.h"
#include "Audioclient.h"
#include "Error.h"

inline void assertHrResult(const HRESULT hr) {
	if (FAILED(hr)) {
		std::string msg;
		switch (hr) {
		case AUDCLNT_E_NOT_INITIALIZED:
			msg = "The audio stream has not been successfully initialized.";
			break;
		case AUDCLNT_E_ALREADY_INITIALIZED:
			msg = "The IAudioClient object is already initialized.";
			break;
		case AUDCLNT_E_WRONG_ENDPOINT_TYPE:
			msg = "The AUDCLNT_STREAMFLAGS_LOOPBACK flag is set but the endpoint device is a capture device, not a rendering device.";
			break;
		case AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED:
			msg = "The requested buffer size is not aligned. This code can be returned for a render or a capture device if the caller specified AUDCLNT_SHAREMODE_EXCLUSIVE and the AUDCLNT_STREAMFLAGS_EVENTCALLBACK flags. The caller must call Initialize again with the aligned buffer size. For more information, see Remarks.";
			break;
		case AUDCLNT_E_BUFFER_SIZE_ERROR:
			msg = "Indicates that the buffer duration value requested by an exclusive-mode client is out of range. The requested duration value for pull mode must not be greater than 500 milliseconds; for push mode the duration value must not be greater than 2 seconds.";
			break;
		case AUDCLNT_E_CPUUSAGE_EXCEEDED:
			msg = "Indicates that the process-pass duration exceeded the maximum CPU usage. The audio engine keeps track of CPU usage by maintaining the number of times the process-pass duration exceeds the maximum CPU usage. The maximum CPU usage is calculated as a percent of the engine's periodicity. The percentage value is the system's CPU throttle value (within the range of 10% and 90%). If this value is not found, then the default value of 40% is used to calculate the maximum CPU usage.";
			break;
		case AUDCLNT_E_DEVICE_INVALIDATED:
			msg = "The audio endpoint device has been unplugged, or the audio hardware or associated hardware resources have been reconfigured, disabled, removed, or otherwise made unavailable for use.";
			break;
		case AUDCLNT_E_DEVICE_IN_USE:
			msg = "The endpoint device is already in use. Either the device is being used in exclusive mode, or the device is being used in shared mode and the caller asked to use the device in exclusive mode.";
			break;
		case AUDCLNT_E_ENDPOINT_CREATE_FAILED:
			msg = "The method failed to create the audio endpoint for the render or the capture device. This can occur if the audio endpoint device has been unplugged, or the audio hardware or associated hardware resources have been reconfigured, disabled, removed, or otherwise made unavailable for use.";
			break;
		case AUDCLNT_E_INVALID_DEVICE_PERIOD:
			msg = "Indicates that the device period requested by an exclusive-mode client is greater than 500 milliseconds.";
			break;
		case AUDCLNT_E_UNSUPPORTED_FORMAT:
			msg = "The audio engine (shared mode) or audio endpoint device (exclusive mode) does not support the specified format.";
			break;
		case AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED:
			msg = "The caller is requesting exclusive-mode use of the endpoint device, but the user has disabled exclusive-mode use of the device.";
			break;
		case AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL:
			msg = "The AUDCLNT_STREAMFLAGS_EVENTCALLBACK flag is set but parameters hnsBufferDuration and hnsPeriodicity are not equal.";
			break;
		case AUDCLNT_E_SERVICE_NOT_RUNNING:
			msg = "The Windows audio service is not running.";
			break;
		case E_POINTER:
			msg = "Parameter pFormat is NULL.";
			break;
		case E_INVALIDARG:
			msg = "Parameter pFormat points to an invalid format description; or the AUDCLNT_STREAMFLAGS_LOOPBACK flag is set but ShareMode is not equal to AUDCLNT_SHAREMODE_SHARED; or the AUDCLNT_STREAMFLAGS_CROSSPROCESS flag is set but ShareMode is equal to AUDCLNT_SHAREMODE_EXCLUSIVE. A prior call to SetClientProperties was made with an invalid category for audio / render streams.";
			break;
		case E_OUTOFMEMORY:
			msg = "Out of memory.";
			break;
		case AUDCLNT_E_OUT_OF_ORDER:
			msg = "A previous IAudioRenderClient::GetBuffer call is still in effect.";
			break;
		case AUDCLNT_E_BUFFER_ERROR:
			msg = "GetBuffer failed to retrieve a data buffer and *ppData points to NULL. For more information, see Remarks.";
			break;
		case AUDCLNT_E_BUFFER_TOO_LARGE:
			msg = "The NumFramesRequested value exceeds the available buffer space (buffer size minus padding size).";
			break;
		case AUDCLNT_E_BUFFER_OPERATION_PENDING:
			msg = "Buffer cannot be accessed because a stream reset is in progress.";
			break;
		case AUDCLNT_E_NOT_STOPPED:
			msg = "The audio stream was not stopped at the time of the Start call.";
			break;
		case AUDCLNT_E_INVALID_SIZE:
			msg = "The NumFramesWritten value exceeds the NumFramesRequested value specified in the previous IAudioRenderClient::GetBuffer call.";
			break;
		case AUDCLNT_E_THREAD_NOT_REGISTERED:
			msg = "The thread is not registered.";
			break;
		case AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED:
			msg = "The audio stream was not initialized for event-driven buffering.";
			break;
		case AUDCLNT_E_EXCLUSIVE_MODE_ONLY:
			msg = "Exclusive mode only.";
			break;
		case AUDCLNT_S_BUFFER_EMPTY:
			msg = "The call succeeded and *pNumFramesToRead is 0, indicating that no capture data is available to be read.";
			break;
		case AUDCLNT_E_EVENTHANDLE_NOT_SET:
			msg = "The audio stream is configured to use event-driven buffering, but the caller has not called IAudioClient::SetEventHandle to set the event handle on the stream.";
			break;
		case AUDCLNT_E_INCORRECT_BUFFER_SIZE:
			msg = "Indicates that the buffer has an incorrect size.";
			break;
		case AUDCLNT_E_INVALID_STREAM_FLAG:
			msg = "AUDCLNT_E_INVALID_STREAM_FLAG";
			break;
		case AUDCLNT_E_ENDPOINT_OFFLOAD_NOT_CAPABLE:
			msg = "AUDCLNT_E_ENDPOINT_OFFLOAD_NOT_CAPABLE";
			break;
		case AUDCLNT_E_OUT_OF_OFFLOAD_RESOURCES:
			msg = "AUDCLNT_E_OUT_OF_OFFLOAD_RESOURCES";
			break;
		case AUDCLNT_E_OFFLOAD_MODE_ONLY:
			msg = "AUDCLNT_E_OFFLOAD_MODE_ONLY";
			break;
		case AUDCLNT_E_NONOFFLOAD_MODE_ONLY:
			msg = "AUDCLNT_E_NONOFFLOAD_MODE_ONLY";
			break;
		case AUDCLNT_E_RESOURCES_INVALIDATED:
			msg = "AUDCLNT_E_RESOURCES_INVALIDATED";
			break;
		case AUDCLNT_E_RAW_MODE_UNSUPPORTED:
			msg = "AUDCLNT_E_RAW_MODE_UNSUPPORTED";
			break;
		case AUDCLNT_S_THREAD_ALREADY_REGISTERED:
			msg = "AUDCLNT_S_THREAD_ALREADY_REGISTERED";
			break;
		case AUDCLNT_S_POSITION_STALLED:
			break;
		default:
			if (hr == E_NOT_SET) {
				msg = "The audio endpoint device was not found.";
			}
		}
		throw Error("WASAPI (0x%08x) %s", hr, msg.c_str());
	}
}

class AudioDevice {
public:
	static void initStatic();
	static void destroyStatic();
	static std::vector<AudioDevice*> getDevices();

	AudioDevice();
	AudioDevice(const std::string &id);
	AudioDevice(const std::wstring &id);
	~AudioDevice();

	const std::string getId();
	const std::string getName();
	const WAVEFORMATEX* getFormat() const;
	UINT32 getBufferFrameCount() const;
	UINT32 getBufferFrameCountAvailable() const;

	IAudioCaptureClient* getCaptureClient();
	IAudioRenderClient* getRenderClient();
	ISimpleAudioVolume* getVolume();
	void start();
	void stop();
	void printInfo();

private:
	static IMMDeviceEnumerator *_pEnumerator;
	static bool _initStatic;

	bool _initMode;
	IMMDevice *_pDevice;
	IAudioClient *_pAudioClient;
	IAudioCaptureClient *_pCaptureClient;
	IAudioRenderClient *_pRenderClient;
	ISimpleAudioVolume *_pSimpleVolume;
	WAVEFORMATEX *_pFormat;
	UINT32 _bufferFrameCount;
	std::string _id, _name;

	AudioDevice(IMMDevice *pDevice);
	void init();
	void init(IMMDevice *pDevice);
	void initMode(const bool capture);

};

