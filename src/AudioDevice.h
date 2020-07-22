/*
	This class represents a single audio endpoint/device/soundcard.
	All operations having to do with capturing or rendering audio is defined here.
	It's implemented for Windows using the WASAPI(Windows Audio Session API).
	If any one wants to port this application to another operating system the majority of work would be on this class.

	Author: Andreas Arvidsson
	Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <vector>
#include <atomic>
#include <string>
#include <memory>
#include "Audioclient.h"
#include "Error.h"

#define assert(hr) AudioDevice::assert(hr)

using std::string;
using std::wstring;
using std::atomic;
using std::vector;
using std::unique_ptr;

struct IMMDevice;
struct IMMDeviceEnumerator;

class AudioDevice {
public:
	static void initStatic();
	static void destroyStatic();
	static vector<string> getDeviceNames();
	static string getDeviceName(IMMDevice *pDevice);
	static unique_ptr<AudioDevice> initDevice(const string &name);
    static void throwError();

	AudioDevice();
	AudioDevice(const string &id);
	AudioDevice(const wstring &id);
	AudioDevice(IMMDevice* pDevice);
	~AudioDevice();

	void initCaptureService();
	void initRenderService();
	void startService();

	const string getId();
	const string getName();
	const WAVEFORMATEX* getFormat() const;
	ISimpleAudioVolume* getVolumeControl();
	void printInfo() const;
	const UINT32 getBufferSize() const;
	const UINT32 getEngineBufferSize() const;
	void flushCaptureBuffer() const;
	void flushRenderBuffer() const;

    inline void static assert(const HRESULT hr) {
        if (FAILED(hr)) {
            _error = Error("WASAPI (0x%08x) %s", hr, hresult(hr).c_str());
            _throwError = true;
        }
    }

	inline const UINT32 getBufferFrameCountAvailable() const {
		static UINT32 numFramesPadding;
		assert(_pAudioClient->GetCurrentPadding(&numFramesPadding));
		return _bufferSize - numFramesPadding;
	}

	inline const HRESULT getNextPacketSize(UINT32 *pNumFramesInNextPacket) const {
		return _pCaptureClient->GetNextPacketSize(pNumFramesInNextPacket);
	}

	inline const HRESULT getCaptureBuffer(float **pCaptureBuffer, UINT32 *pNumFramesToRead, DWORD *pFlags, UINT64 *pPosition) const {
		return _pCaptureClient->GetBuffer((BYTE**)pCaptureBuffer, pNumFramesToRead, pFlags, pPosition, nullptr);
	}

	inline const HRESULT getCaptureBuffer(float **pCaptureBuffer, UINT32 *pNumFramesToRead, DWORD *pFlags) const {
		return _pCaptureClient->GetBuffer((BYTE**)pCaptureBuffer, pNumFramesToRead, pFlags, nullptr, nullptr);
	}

	inline const HRESULT getCaptureBuffer(float **pCaptureBuffer, UINT32 *pNumFramesToRead) const {
		static DWORD flags;
		return _pCaptureClient->GetBuffer((BYTE**)pCaptureBuffer, pNumFramesToRead, &flags, nullptr, nullptr);
	}

	inline const HRESULT getRenderBuffer(float **pRenderBuffer, const UINT32 numFramesRequested) const {
		return _pRenderClient->GetBuffer(numFramesRequested, (BYTE**)pRenderBuffer);
	}

	inline const HRESULT releaseCaptureBuffer(const UINT32 numFramesRead) const {
		return _pCaptureClient->ReleaseBuffer(numFramesRead);
	}

	inline const HRESULT releaseRenderBuffer(const UINT32 numFramesWritten) const {
		return _pRenderClient->ReleaseBuffer(numFramesWritten, 0);
	}

	inline const HRESULT releaseRenderBuffer(const UINT32 numFramesWritten, DWORD flags) const {
		return _pRenderClient->ReleaseBuffer(numFramesWritten, flags);
	}

	inline const HANDLE getEventHandle() const {
		return _eventHandle;
	}

private:
	static IMMDeviceEnumerator *_pEnumerator;
	static bool _initStatic;
    static atomic<bool> _throwError;
    static Error _error;

	IMMDevice *_pDevice;
	IAudioClient3 *_pAudioClient;
	IAudioCaptureClient *_pCaptureClient;
	IAudioRenderClient *_pRenderClient;
	ISimpleAudioVolume *_pSimpleVolume;
	WAVEFORMATEX *_pFormat;
	UINT32 _bufferSize, _engineBufferSize;
	string _id, _name;
	HANDLE _eventHandle;

    static const string hresult(const HRESULT hr);

	void initDefault();
	void init(IMMDevice *pDevice);
	void prepareService(const bool capture);
};