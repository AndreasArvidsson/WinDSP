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
#include "Mmdeviceapi.h"
#include "Audioclient.h"
#include "ErrorMessages.h"

class AudioDevice {
public:
	static void initStatic();
	static void destroyStatic();
	static std::vector<std::string> getDeviceNames();
	static std::string getDeviceName(IMMDevice *pDevice);
	static AudioDevice* initDevice(const std::string &name);

	AudioDevice();
	AudioDevice(const std::string &id);
	AudioDevice(const std::wstring &id);
	~AudioDevice();

	void startCaptureService();
	void startRenderService();

	const std::string getId();
	const std::string getName();
	const WAVEFORMATEX* getFormat() const;
	ISimpleAudioVolume* getVolumeControl();

	inline const UINT32 getBufferSize() const {
		return _bufferSize;
	}

	inline const UINT32 getBufferFrameCountAvailable() const {
		static UINT32 numFramesPadding;
		assert(_pAudioClient->GetCurrentPadding(&numFramesPadding));
		return _bufferSize - numFramesPadding;
	}

	inline const HRESULT getNextPacketSize(UINT32 *pNumFramesInNextPacket) const {
		return _pCaptureClient->GetNextPacketSize(pNumFramesInNextPacket);
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

	inline void flushCaptureBuffer() const {
		UINT32 frameCount;
		static BYTE *buffer;
		static DWORD flags;
		assert(_pCaptureClient->GetBuffer(&buffer, &frameCount, &flags, nullptr, nullptr));
		assert(_pCaptureClient->ReleaseBuffer(frameCount));
	}

	inline void flushRenderBuffer() const {
		static BYTE *buffer;
		const UINT32 frameCount = getBufferFrameCountAvailable();
		assert(_pRenderClient->GetBuffer(frameCount, &buffer));
		assert(_pRenderClient->ReleaseBuffer(frameCount, AUDCLNT_BUFFERFLAGS_SILENT));
	}

	inline const HANDLE getEventHandle() const {
		return _eventHandle;
	}

private:
	static IMMDeviceEnumerator *_pEnumerator;
	static bool _initStatic;

	IMMDevice *_pDevice;
	IAudioClient3 *_pAudioClient;
	IAudioCaptureClient *_pCaptureClient;
	IAudioRenderClient *_pRenderClient;
	ISimpleAudioVolume *_pSimpleVolume;
	WAVEFORMATEX *_pFormat;
	UINT32 _bufferSize;
	std::string _id, _name;

	AudioDevice(IMMDevice *pDevice);
	void initDefault();
	void init(IMMDevice *pDevice);
	void startService(const bool capture);

	HANDLE _eventHandle;

};

