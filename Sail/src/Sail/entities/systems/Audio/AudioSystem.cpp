#include "pch.h"

#include "AudioSystem.h"

#include "..//Sail/src/API/Audio/AudioEngine.h"
#include "..//Sail/src/Sail/Application.h"
#include "..//Sail/src/Sail/entities/components/AudioComponent.h"
#include "..//Sail/src/Sail/entities/components/TransformComponent.h"
#include "..//Sail/src/Sail/graphics/camera/Camera.h"
#include "..//..//Entity.h"

#include <iterator>

AudioSystem::AudioSystem() : BaseComponentSystem() {
	registerComponent<AudioComponent>(true, true, true);
	registerComponent<TransformComponent>(true, true, false);

	m_audioEngine = SAIL_NEW AudioEngine();
	initialize();
}

AudioSystem::~AudioSystem() {
	m_audioEngine->stopAllSounds();
	delete m_audioEngine;
}

// TODO: move to constructor?
void AudioSystem::initialize() {
	m_audioEngine->loadSound("../Audio/footsteps_1.wav");
	m_audioEngine->loadSound("../Audio/jump.wav");
	m_audioEngine->loadSound("../Audio/guitar.wav");
}

void AudioSystem::update(Camera& cam, float dt, float alpha) {
	for (auto e : entities) {
		auto audioC = e->getComponent<AudioComponent>();
		// Loop through sounds
		for (int i = 0; i < Audio::SoundType::COUNT; i++) {
			Audio::SoundInfo& sound = audioC->m_sounds[i]; // To make the code easier to read

			if (sound.isPlaying) {
				// Initialize the sound if that hasn't been done already
				if (!sound.isInitialized) {
					sound.soundID = m_audioEngine->initializeSound(sound.fileName, sound.volume);
					sound.isInitialized = true;
					sound.soundEffectTimer = 0.0f;
				}

				// Start playing the sound if it's not already playing
				if (sound.soundEffectTimer == 0.0f) {
					m_audioEngine->startSpecificSound(sound.soundID);
				}

				// Update the sound with the current positions if it's playing
				if (sound.soundEffectTimer <= sound.soundEffectLength) {
					m_audioEngine->updateSoundWithCurrentPosition(
						sound.soundID, cam, *e->getComponent<TransformComponent>(), 
						sound.positionalOffset, alpha);					
					
					sound.soundEffectTimer += dt;
				} else {
					sound.soundEffectTimer = 0.0f; // Reset the sound effect to its beginning
					m_audioEngine->stopSpecificSound(sound.soundID);
					
					// If the sound isn't looping then make it stop
					if (sound.playOnce == true) {
						sound.isPlaying = false;
						sound.isInitialized = false;
					}
				}
			// If the sound should no longer be playing stop it and reset its timer
			} else if (sound.soundEffectTimer != 0.0f) {
				m_audioEngine->stopSpecificSound(sound.soundID);
				sound.soundEffectTimer = 0.0f;
			}
		}
	}


	// STREAMING: This part has not been tested in a while
	{
		//		// Playing STREAMED sounds
		//	std::list<std::pair<std::string, bool>>::iterator i;
		//	std::list<std::pair<std::string, bool>>::iterator toBeDeleted;
		//	std::list<std::pair<std::string, int>>::iterator j;
		//	std::list<std::pair<std::string, int>>::iterator streamToBeDeleted;
		//	std::string filename = "";
		//	int streamIndex = 0;

		//	for (i = audioC->m_streamingRequests.begin(); i != audioC->m_streamingRequests.end();) {

		//		if (i->second == true) {

		//			filename = i->first;
		//			toBeDeleted = i;
		//			i++;

		//			streamIndex = m_audioEngine.getAvailableStreamIndex();

		//			if (streamIndex == -1) {
		//				Logger::Error("Too many sounds already streaming; failed to stream another one!");
		//			}
		//			else {

		//				Application::getInstance()->pushJobToThreadPool(
		//					[this, filename, streamIndex](int id) {
		//						return m_audioEngine.streamSound(filename, streamIndex);
		//					});

		//				audioC->m_currentlyStreaming.emplace_back(filename, streamIndex);
		//				audioC->m_streamingRequests.erase(toBeDeleted);
		//			}
		//		}
		//		else/*if (i.second == false)*/ {

		//			filename = i->first;
		//			toBeDeleted = i;
		//			i++;

		//			for (j = audioC->m_currentlyStreaming.begin(); j != audioC->m_currentlyStreaming.end();) {

		//				streamToBeDeleted = j;
		//				j++;

		//				if (streamToBeDeleted->first == filename) {

		//					bool expectedValue = false;
		//					while (!m_audioEngine.m_streamLocks[streamToBeDeleted->second].compare_exchange_strong(expectedValue, true));

		//					m_audioEngine.stopSpecificStream(streamToBeDeleted->second);
		//					audioC->m_currentlyStreaming.erase(streamToBeDeleted);

		//					break;
		//				}
		//			}
		//			audioC->m_streamingRequests.erase(toBeDeleted);
		//		}
		//	}
		//}
	}
}


void AudioSystem::stop() {
	m_audioEngine->stopAllStreams();
}

AudioEngine* AudioSystem::getAudioEngine() {
	return m_audioEngine;
}