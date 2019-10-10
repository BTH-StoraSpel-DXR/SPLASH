#pragma once

#include "Sail/entities/components/Component.h"
#include <string>
#include <stack>

namespace Audio {
	enum SoundType{
		AMBIENT,
		WALK, 
		RUN, 
		SHOOT, 
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
		glm::vec3 positionalOffset = { 0.f, 0.f, 0.f };
		int soundID = -1;
	};
}

class AudioComponent : public Component<AudioComponent>
{
public:
	AudioComponent();
	virtual ~AudioComponent();

	Audio::SoundInfo m_sounds[Audio::SoundType::COUNT]{};

	// • string = filename
	// • bool = TRUE if START-request, FALSE if STOP-request
	std::list<std::pair<std::string, bool>> m_streamingRequests;
	// • string = filename
	// • int = ID of playing streaming; needed for STOPPING the streamed sound
	std::list<std::pair<std::string, int>> m_currentlyStreaming;

	// This function is purely here to MAKE LIFE LESS DIFFICULT
	void defineSound(Audio::SoundType type, Audio::SoundInfo info);
};

