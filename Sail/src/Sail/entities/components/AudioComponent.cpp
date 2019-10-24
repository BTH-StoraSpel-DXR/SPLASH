#include "pch.h"

#include "AudioComponent.h"
#include "../../utils/Utils.h"

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