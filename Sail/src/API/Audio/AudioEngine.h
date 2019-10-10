#pragma once

#include <xaudio2.h>
#include <xapo.h>
#include <hrtfapoapi.h>
#include <wrl/client.h>

#define SOUND_COUNT 236
#define STREAMED_SOUNDS_COUNT 20
#define STREAMING_BUFFER_SIZE 32768
#define MAX_BUFFER_COUNT 3
#define VOL_HALF 0.5f
#define VOL_THIRD 0.33f
#define VOL_FOURTH 0.25f

class Camera;
class Transform;

enum AudioType { MUSIC };

struct StreamingVoiceContext : public IXAudio2VoiceCallback {
	STDMETHOD_(void, OnVoiceProcessingPassStart)(UINT32) override {}
	STDMETHOD_(void, OnVoiceProcessingPassEnd)()         override {}
	STDMETHOD_(void, OnStreamEnd)()                      override {}
	STDMETHOD_(void, OnBufferStart)(void*)               override {}
	STDMETHOD_(void, OnBufferEnd)(void*)                 override { SetEvent(hBufferEndEvent); }
	STDMETHOD_(void, OnLoopEnd)(void*)                   override {}
	STDMETHOD_(void, OnVoiceError)(void*, HRESULT)       override {}

	HANDLE hBufferEndEvent;

#pragma region STREAMING_VOICE_CONTEXT
	StreamingVoiceContext() :
#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
		hBufferEndEvent(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE))
#else
		hBufferEndEvent(CreateEvent(nullptr, FALSE, FALSE, nullptr))
#endif
	{}
	virtual ~StreamingVoiceContext() { CloseHandle(hBufferEndEvent); }
};
#pragma endregion


struct soundStruct {
	std::string          filename = { "" };
	IXAudio2SourceVoice* sourceVoice = nullptr;
	HrtfEnvironment      environment = HrtfEnvironment::Outdoors;
	Microsoft::WRL::ComPtr<IXAPOHrtfParameters> hrtfParams;
};

class AudioEngine
{
public:
	AudioEngine();
	~AudioEngine();

	void loadSound(const std::string& filename);
	int initializeSound(const std::string& filename, float volume = 1.0f);
	void streamSound(const std::string& filename, int streamIndex, bool loop = true);

	void updateSoundWithCurrentPosition(int index, Camera& cam, const Transform& transform, 
		const glm::vec3& positionOffset, float alpha);

	void startSpecificSound(int index, float volume = 1.0f);
	void stopSpecificSound(int index);
	void stopSpecificStream(int index);
	void stopAllStreams();
	void stopAllSounds();

	float getSoundVolume(int index);
	float getStreamVolume(int index);
	int getSoundIndex();
	int getAvailableStreamIndex();

	void setSoundVolume(int index, float value = VOL_HALF);
	void setStreamVolume(int index, float value = VOL_HALF);

	std::atomic<bool> m_streamLocks[STREAMED_SOUNDS_COUNT];

private: 
	bool m_isRunning = true;

	// The main audio 'core'
	IXAudio2* m_xAudio2 = nullptr;	

	// Represents the audio output device
	IXAudio2MasteringVoice* m_masterVoice = nullptr;
	DWORD m_destinationChannelCount;
	// Represents each loaded sound in the form of an 'object'
	soundStruct m_sound[SOUND_COUNT];
	soundStruct m_stream[STREAMED_SOUNDS_COUNT];

	int m_currSoundIndex = 0;
	float m_tempDistance = 0;
	//std::atomic<int> m_currStreamIndex = 0;

	void initialize();

	BYTE m_streamBuffers[MAX_BUFFER_COUNT][STREAMING_BUFFER_SIZE];
	bool m_isStreaming[STREAMED_SOUNDS_COUNT];
	bool m_isFinished[STREAMED_SOUNDS_COUNT];
	OVERLAPPED m_overlapped[STREAMED_SOUNDS_COUNT];

	// PRIVATE FUNCTION
	//-----------------
	HRESULT initXAudio2();

	void streamSoundInternal(const std::string& filename, int myIndex, bool loop);
	HRESULT FindMediaFileCch(WCHAR* strDestPath, int cchDest, LPCWSTR strFilename);

	bool checkSoundIndex(int index);
	bool checkStreamIndex(int index);
	// ----------------
};
