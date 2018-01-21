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
	static std::vector<AudioDevice*> getDevices();

	AudioDevice();
	AudioDevice(const std::string &id);
	AudioDevice(const std::wstring &id);
	~AudioDevice();

	void startCaptureService();
	void startRenderService();

	const std::string getId();
	const std::string getName();
	const WAVEFORMATEX* getFormat() const;
	UINT32 getBufferFrameCount() const;
	UINT32 getBufferFrameCountAvailable() const;
	ISimpleAudioVolume* getVolumeControl();

	inline const HRESULT getNextPacketSize(UINT32 *pNumFramesInNextPacket) const {
		return  _pCaptureClient->GetNextPacketSize(pNumFramesInNextPacket);
	}

	inline const HRESULT getCaptureBuffer(float **pCaptureBuffer, UINT32 *pNumFramesToRead) const {
		DWORD flags;
		return  _pCaptureClient->GetBuffer((BYTE**)pCaptureBuffer, pNumFramesToRead, &flags, NULL, NULL);
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

private:
	static IMMDeviceEnumerator *_pEnumerator;
	static bool _initStatic;

	IMMDevice *_pDevice;
	IAudioClient *_pAudioClient;
	IAudioCaptureClient *_pCaptureClient;
	IAudioRenderClient *_pRenderClient;
	ISimpleAudioVolume *_pSimpleVolume;
	WAVEFORMATEX *_pFormat;
	UINT32 _bufferFrameCount;
	std::string _id, _name;

	AudioDevice(IMMDevice *pDevice);
	void initDefault();
	void init(IMMDevice *pDevice);
	void startService(const bool capture);

};

