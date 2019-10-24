#include "pch.h"

#include "AudioComponent.h"
#include "../../utils/Utils.h"
#include "Sail.h"


void Audio::Factory::defineSoundsPlayer(Entity* player) {
	Audio::SoundInfo sound{};
	sound.fileName = "../Audio/footsteps_1.wav";
	sound.soundEffectLength = 1.0f;
	sound.volume = 0.5f;
	sound.playOnce = false;
	sound.positionalOffset = { 0.0f, -1.6f, 0.0f };
	player->getComponent<AudioComponent>()->defineSound(Audio::SoundType::RUN, sound);

	sound.fileName = "../Audio/jump.wav";
	sound.soundEffectLength = 0.7f;
	sound.playOnce = true;
	sound.positionalOffset = { 0.0f, 0.0f, 0.0f };
	player->getComponent<AudioComponent>()->defineSound(Audio::SoundType::JUMP, sound);

	sound.fileName = "../Audio/watergun_start.wav";
	sound.soundEffectLength = 0.578f;
	sound.volume = 1.0f;
	sound.playOnce = true;
	sound.positionalOffset = { 0.5f, -0.5f, 0.0f };
	player->getComponent<AudioComponent>()->defineSound(Audio::SoundType::SHOOT_START, sound);

	sound.fileName = "../Audio/watergun_loop.wav";
	sound.soundEffectLength = 1.4f;
	sound.playOnce = false;
	sound.volume = 1.0f;
	sound.positionalOffset = { 0.5f, -0.5f, 0.0f };
	player->getComponent<AudioComponent>()->defineSound(Audio::SoundType::SHOOT_LOOP, sound);

	sound.fileName = "../Audio/watergun_end.wav";
	sound.soundEffectLength = 0.722f;
	sound.playOnce = true;
	sound.volume = 1.0f;
	sound.positionalOffset = { 0.5f, -0.5f, 0.0f };
	player->getComponent<AudioComponent>()->defineSound(Audio::SoundType::SHOOT_END, sound);

	sound.fileName = "../Audio/water_drip_1.wav";
	sound.playOnce = true;
	sound.positionalOffset = { 0.0f, 0.0f, 0.0f };
	player->getComponent<AudioComponent>()->defineSound(Audio::SoundType::WATER_IMPACT_LEVEL, sound);

	sound.fileName = "../Audio/water_impact_enemy.wav";
	sound.playOnce = true;
	sound.positionalOffset = { 0.0f, 0.0f, 0.0f };
	player->getComponent<AudioComponent>()->defineSound(Audio::SoundType::WATER_IMPACT_ENEMY, sound);

	sound.fileName = "../Audio/water_impact_my_candle.wav";
	sound.playOnce = true;
	sound.positionalOffset = { 0.0f, 0.0f, 0.0f };
	player->getComponent<AudioComponent>()->defineSound(Audio::SoundType::WATER_IMPACT_MY_CANDLE, sound);
}

AudioComponent::AudioComponent() {}

AudioComponent::~AudioComponent() {}

void AudioComponent::streamSoundRequest_HELPERFUNC(std::string filename, bool startTRUE_stopFALSE, float volume, bool isPositionalAudio, bool isLooping) {

	Audio::StreamRequestInfo info;
	info.startTRUE_stopFALSE = startTRUE_stopFALSE;
	info.volume = volume;
	info.isPositionalAudio = isPositionalAudio;
	info.isLooping = isLooping;

	m_streamingRequests.push_back(std::pair(filename, info));
}

void AudioComponent::defineSoundGeneral(Audio::SoundType type, Audio::SoundInfo_General info) {
	m_sounds[type] = info;
}

void AudioComponent::defineSoundUnique(Audio::SoundType type, Audio::SoundInfo_Unique info) {
	
	bool alreadyExists = false;

	for (std::vector<Audio::SoundInfo_Unique>::iterator i = m_soundsUnique->begin(); i != m_soundsUnique->end(); i++) {

		if (i->fileName == info.fileName) {
			alreadyExists = true;
		}
	}

	if (alreadyExists) {
		Logger::Error("Tried to define a sound that already exists!");
	}
	else {
		m_soundsUnique[type].push_back(info);
	}
}