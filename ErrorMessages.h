/*
	This file contains the handling of Operating system error messages.
	Validation and plain text descriptions.

	Author: Andreas Arvidsson
	Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <string>
#include "Audioclient.h"
#include "asio/asiosys.h"
#include "asio/asio.h"
#include "Error.h"

class ErrorMessages {
public:

	//Convert HRESULT to text explanation.
	static const std::string hresult(const HRESULT hr) {
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
		default:
			if (hr == E_NOT_SET) {
				return "The audio endpoint device was not found.";
			}
			return "Unknown error";
		}
	}

	//Convert ASIOError to text explanation.
	static const std::string asioResult(const ASIOError error) {
		switch (error) {
		case ASE_OK:
			return "This value will be returned whenever the call succeeded";
		case ASE_SUCCESS:
			return "unique success return value for ASIOFuture calls";
		case ASE_NotPresent:
			return "hardware input or output is not present or available";
		case ASE_HWMalfunction:
			return "hardware is malfunctioning (can be returned by any ASIO function)";
		case ASE_InvalidParameter:
			return "input parameter invalid";
		case ASE_InvalidMode:
			return "hardware is in a bad mode or used in a bad mode";
		case ASE_SPNotAdvancing:
			return "hardware is not running when sample position is inquired";
		case ASE_NoClock:
			return "sample clock or rate cannot be determined or is not present";
		case ASE_NoMemory:
			return "not enough memory for completing the request";
		default:
			return "Unknown error";
		}
	}

	static const std::string waitResult(const DWORD error) {
		switch (error) {
		case WAIT_ABANDONED:
			return "WAIT_ABANDONED: The specified object is a mutex object that was not released by the thread that owned the mutex object before the owning thread terminated. Ownership of the mutex object is granted to the calling thread and the mutex is set to nonsignaled. If the mutex was protecting persistent state information, you should check it for consistency.";
		case WAIT_IO_COMPLETION:
			return "WAIT_IO_COMPLETION: The wait was ended by one or more user-mode asynchronous procedure calls (APC) queued to the thread.";
		case WAIT_OBJECT_0:
			return "WAIT_OBJECT_0: The state of the specified object is signaled.";
		case WAIT_TIMEOUT:
			return "WAIT_TIMEOUT: The time-out interval elapsed, and the object's state is nonsignaled.";
		case WAIT_FAILED:
			return "WAIT_FAILED: The function has failed. To get extended error information, call GetLastError.";
		default:
			return "Unknown error";
		}
	}

	static const std::string asioSampleType(const ASIOSampleType type) {
		switch (type) {
		case ASIOSTInt16MSB: return "Int16MSB";
		case ASIOSTInt24MSB: return "Int24MSB";
		case ASIOSTInt32MSB: return "Int32MSB";
		case ASIOSTFloat32MSB: return "Float32MSB";
		case ASIOSTFloat64MSB: return "Float64MSB";
		case ASIOSTInt32MSB16: return "Int32MSB16";
		case ASIOSTInt32MSB18: return "Int32MSB18";
		case ASIOSTInt32MSB20: return "Int32MSB20";
		case ASIOSTInt32MSB24: return "Int32MSB24";
		case ASIOSTInt16LSB: return "Int16LSB";
		case ASIOSTInt24LSB: return "Int24LSB";
		case ASIOSTInt32LSB: return "Int32LSB";
		case ASIOSTFloat32LSB: return "Float32LSB";
		case ASIOSTFloat64LSB: return "Float64LSB";
		case ASIOSTInt32LSB16: return "Int32LSB16";
		case ASIOSTInt32LSB18: return "Int32LSB18";
		case ASIOSTInt32LSB20: return "Int32LSB20";
		case ASIOSTInt32LSB24: return "Int32LSB24";
		case ASIOSTDSDInt8LSB1: return "DSDInt8LSB1";
		case ASIOSTDSDInt8MSB1: return "DSDInt8MSB1";
		case ASIOSTDSDInt8NER8: return "DSDInt8NER8";
		default: return std::to_string(type);
		}
	}

};

inline void assert(const HRESULT hr) {
	if (FAILED(hr)) {
		throw Error("WASAPI (0x%08x) %s", hr, ErrorMessages::hresult(hr).c_str());
	}
}

inline void assertAsio(const ASIOError value) {
	if (value != ASE_OK) {
		throw Error("ASIO (0x%08x) %s", value, ErrorMessages::asioResult(value).c_str());
	}
}

inline void assertWait(const DWORD value) {
	if (value != WAIT_OBJECT_0) {
		throw Error("WAIT (0x%08x) %s", value, ErrorMessages::waitResult(value).c_str());
	}
}

inline void assertSampleType(const ASIOSampleType type) {
	if (type != ASIOSTInt32LSB) {
		throw new Error("ASIO: Unhandled sample type %s", ErrorMessages::asioSampleType(type));
	}
}

inline void assertTrue(const bool value, const std::string &msg) {
	if (!value) {
		throw Error(msg.c_str());
	}
}

