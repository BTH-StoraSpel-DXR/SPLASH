#include "pch.h"

#include "AudioComponent.h"

AudioComponent::AudioComponent() {}

AudioComponent::~AudioComponent() {}

void AudioComponent::defineSound(Audio::SoundType type, Audio::SoundInfo info) {
	m_sounds[type] = info;
}
