#ifndef AUDIODATA_H
#define AUDIODATA_H

#include <string>
#include "glm/glm.hpp"
#include <vector>

namespace Audio {
	enum SoundType {
		RUN_METAL,
		RUN_TILE,
		RUN_WATER_METAL,
		RUN_WATER_TILE,
		SHOOT_START,
		SHOOT_LOOP,
		SHOOT_END,
		RELOAD,
		WATER_IMPACT_ENVIRONMENT,
		WATER_IMPACT_ENEMY_CANDLE,
		WATER_IMPACT_MY_CANDLE,
		JUMP,
		LANDING_GROUND,
		DEATH_ME,
		DEATH_OTHER,
		COUNT // Keep at the bottom so that COUNT == nr of sound types
	};

	// One for every SOUND TYPE
	struct SoundInfo_General {
		bool hasStartedPlaying = false;
		bool isPlaying = false;
		bool playOnce = true;
		float durationElapsed = 0.0f;
		float currentSoundsLength = 0.0f;
		// Cam HEIGHT (y-pos) is 1.6f; useful info for incorporating positonalOffset
		glm::vec3 positionalOffset = { 0.f, 0.f, 0.f };
		int soundID = -1;
		int prevRandomNum = 0;
	};

	// One for LITERALLY EVERY SOUND
	struct SoundInfo_Unique {
		std::string fileName = "";
		float volume = 1.0f;
		float soundEffectLength = 0.0f;
	};

	struct StreamRequestInfo {
		bool startTRUE_stopFALSE;
		float volume;
		bool isPositionalAudio;
		bool isLooping;
	};
}

class AllAudioData {
public:
	AllAudioData() { init(); };
	~AllAudioData() {};
	void init();

	// This data is copied over to each audio component
	Audio::SoundInfo_General m_sounds[Audio::SoundType::COUNT];
	// This data is NOT copied over to each audio component
	std::vector<Audio::SoundInfo_Unique> m_soundsUnique[Audio::SoundType::COUNT];
};

static AllAudioData audioData;

#endif