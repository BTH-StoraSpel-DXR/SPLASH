#include "pch.h"

#include "AudioComponent.h"
#include "../../utils/Utils.h"

AudioComponent::AudioComponent() {

	for (int i = 0; i < Audio::SoundType::COUNT; i++) {

		m_sounds[i] = audioData.m_sounds[i];
	}
}

AudioComponent::~AudioComponent() {}

void AudioComponent::streamSoundRequest_HELPERFUNC(std::string filename, bool startTRUE_stopFALSE, float volume, bool isPositionalAudio, bool isLooping) {

	Audio::StreamRequestInfo info;
	info.startTRUE_stopFALSE = startTRUE_stopFALSE;
	info.volume = volume;
	info.isPositionalAudio = isPositionalAudio;
	info.isLooping = isLooping;

	m_streamingRequests.push_back(std::pair(filename, info));
}
