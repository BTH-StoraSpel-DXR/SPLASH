#pragma once

#include "Sail/entities/components/Component.h"
#include <string>
#include <stack>

class Entity;

namespace Audio {
	enum SoundType{
		AMBIENT,
		WALK, 
		RUN, 
		SHOOT_START,
		SHOOT_LOOP,
		SHOOT_END,
		WATER_IMPACT_LEVEL,
		WATER_IMPACT_ENEMY,
		WATER_IMPACT_MY_CANDLE,
		JUMP, 
		LANDING, 
		COUNT // Keep at the bottom so that COUNT == nr of sound types
	};

	struct SoundInfo {
		std::string fileName = "";
		bool isPlaying = false;
		bool playOnce = true;
		bool isInitialized = false;
		float volume = 1.0f;
		float soundEffectTimer = 0.0f;
		float soundEffectLength = 0.0f;
		// Cam HEIGHT (y-pos) is 1.6f; useful info for incorporating positonalOffset
		glm::vec3 positionalOffset = { 0.f, 0.f, 0.f };
		int soundID = -1;
	};

	struct StreamRequestInfo {
		bool startTRUE_stopFALSE;
		float volume;
		bool isPositionalAudio;
		bool isLooping;
	};

	namespace Factory {
		void defineSoundsPlayer(Entity* pEntity);
	}
}

class AudioComponent : public Component<AudioComponent>
{
public:
	AudioComponent();
	virtual ~AudioComponent();

	Audio::SoundInfo m_sounds[Audio::SoundType::COUNT]{};

	// An 'easy-mode' helper function for starting/stopping a streamed sound
	void streamSoundRequest_HELPERFUNC(std::string filename, bool startTrue_stopFalse, float volume, bool isPositionalAudio, bool isLooping);
	// A helpful function that simplifies the process of defining a new sound
	void defineSound(Audio::SoundType type, Audio::SoundInfo info);

	// VARIABLE DEFINITIONS/CLARIFICATIONS
		// • string = filename
		// • bool = TRUE if START-request, FALSE if STOP-request
	std::list<std::pair<std::string, Audio::StreamRequestInfo>> m_streamingRequests;
	// VARIABLE DEFINITIONS/CLARIFICATIONS
		// • string = filename
		// • int = ID of playing streaming; needed for STOPPING the streamed sound
	std::list<std::pair<std::string, std::pair<int, bool>>> m_currentlyStreaming;
};
