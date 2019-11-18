#pragma once

#include <xaudio2.h>
#include <xapo.h>
#include <hrtfapoapi.h>
#include <wrl/client.h>

#include "Sail/entities/systems/Audio/AudioData.h"

#define SOUND_COUNT 50
#define STREAMED_SOUNDS_COUNT 5
#define STREAMING_BUFFER_SIZE 32768
#define MAX_BUFFER_COUNT 3
#define VOL_HALF 0.5f
#define VOL_THIRD 0.33f
#define VOL_FOURTH 0.25f

class Camera;
class Transform;
class AudioComponent;

enum AudioType { MUSIC };

struct XAUDIO2FX_REVERB_PARAMETERS;

#define X3DAUDIO_PI  3.141592654f

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
	Microsoft::WRL::ComPtr<IXAPO> xapo;
	IXAudio2SourceVoice* sourceVoice = nullptr;
	IXAudio2SubmixVoice* xAPOsubMixVoice = nullptr;
	HrtfEnvironment      environment = HrtfEnvironment::Outdoors;
	Microsoft::WRL::ComPtr<IXAPOHrtfParameters> hrtfParams;
};

class AudioEngine
{
public:
	AudioEngine();
	~AudioEngine();

	void loadSound(const std::string& filename);
	int beginSound(const std::string& filename, Audio::EffectType effectType, float frequency, float volume = 1.0f);
	void streamSound(const std::string& filename, int streamIndex, float volume, bool isPositionalAudio, bool loop = true, AudioComponent* pAudioC = nullptr);

	void updateSoundWithCurrentPosition(int index, Camera& cam, const Transform& transform, 
		const glm::vec3& positionOffset, float alpha);
	void updateStreamWithCurrentPosition(int index, Camera& cam, const Transform& transform,
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
	soundStruct* getSound(int index);
	soundStruct* getStream(int index);

	void setSoundVolume(int index, float value = VOL_HALF);
	void setStreamVolume(int index, float value = VOL_HALF);

	std::atomic<bool> m_streamLocks[STREAMED_SOUNDS_COUNT];

	void updateProjectileLowPass(float frequency, int indexToSource);

private: 
	bool m_isRunning = true;

	// The main audio 'core'
	IXAudio2* m_xAudio2 = nullptr;
	bool m_initFailed = false;

	// Represents the audio output device
	IXAudio2MasteringVoice* m_masterVoice = nullptr;
	IXAudio2SubmixVoice* m_masterSubmixVoice = nullptr;

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

	// PRIVATE FUNCTIONS
	//-----------------
	HRESULT initXAudio2();

	//
	int fetchSoundIndex();

	//
	void sendVoiceTo(IXAudio2SourceVoice* *source, IXAudio2Voice* *destination, bool useFilter);
	void sendVoiceTosubMixVoice(IXAudio2SourceVoice** source, IXAudio2SubmixVoice** destination, bool useFilter);
	void addLowPassFilterTo(IXAudio2SourceVoice* source, IXAudio2Voice* destination, float frequency);
	void setUpxAPO(int indexValue);

	//
	XAUDIO2_EFFECT_DESCRIPTOR createXAPPOEffect(Microsoft::WRL::ComPtr<IXAPO> xapo);
	XAUDIO2_FILTER_PARAMETERS createLowPassFilter(float cutoffFrequence);
	void createXAPOsubMixVoice(IXAudio2SubmixVoice* *source, Microsoft::WRL::ComPtr<IXAPO> xapo); // Automatically points to masterVoice

	void streamSoundInternal(const std::string& filename, int myIndex, float volume, bool isPositionalAudio, bool loop, AudioComponent* pAudioC = nullptr);
	HRESULT FindMediaFileCch(WCHAR* strDestPath, int cchDest, LPCWSTR strFilename);

	bool checkSoundIndex(int index);
	bool checkStreamIndex(int index);
	// ----------------
};
