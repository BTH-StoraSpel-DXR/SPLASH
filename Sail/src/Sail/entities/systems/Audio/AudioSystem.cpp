#include "pch.h"

#include "AudioSystem.h"

#include "..//Sail/src/API/Audio/AudioEngine.h"
#include "../Sail/src/Sail/entities/systems/Audio/AudioData.h"
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

// TO DO: move to constructor?
void AudioSystem::initialize() {
#pragma region FOOTSTEPS
	m_audioEngine->loadSound("../Audio/footsteps_metal_1.wav");
	m_audioEngine->loadSound("../Audio/footsteps_metal_2.wav");
	m_audioEngine->loadSound("../Audio/footsteps_metal_3.wav");
	m_audioEngine->loadSound("../Audio/footsteps_metal_4.wav");
	m_audioEngine->loadSound("../Audio/footsteps_tile_1.wav");
	m_audioEngine->loadSound("../Audio/footsteps_tile_2.wav");
	m_audioEngine->loadSound("../Audio/footsteps_tile_3.wav");
	m_audioEngine->loadSound("../Audio/footsteps_tile_4.wav");
	m_audioEngine->loadSound("../Audio/footsteps_water_metal_1.wav");
	m_audioEngine->loadSound("../Audio/footsteps_water_metal_2.wav");
	m_audioEngine->loadSound("../Audio/footsteps_water_metal_3.wav");
	m_audioEngine->loadSound("../Audio/footsteps_water_metal_4.wav");
	m_audioEngine->loadSound("../Audio/footsteps_water_tile_1.wav");
	m_audioEngine->loadSound("../Audio/footsteps_water_tile_2.wav");
	m_audioEngine->loadSound("../Audio/footsteps_water_tile_3.wav");
	m_audioEngine->loadSound("../Audio/footsteps_water_tile_4.wav");
#pragma endregion
#pragma region JUMPING
	m_audioEngine->loadSound("../Audio/jump.wav");
	m_audioEngine->loadSound("../Audio/landing_ground.wav");
#pragma endregion
#pragma region WATERGUN
	m_audioEngine->loadSound("../Audio/watergun_start.wav");
	m_audioEngine->loadSound("../Audio/watergun_loop.wav");
	m_audioEngine->loadSound("../Audio/watergun_end.wav");
	m_audioEngine->loadSound("../Audio/watergun_reload.wav");
#pragma endregion
#pragma region IMPACTS
	m_audioEngine->loadSound("../Audio/water_impact_enemy_candle.wav");
	m_audioEngine->loadSound("../Audio/water_impact_my_candle.wav");
	m_audioEngine->loadSound("../Audio/water_drip_1.wav");
	m_audioEngine->loadSound("../Audio/water_drip_2.wav");
	m_audioEngine->loadSound("../Audio/water_drip_3.wav");
	m_audioEngine->loadSound("../Audio/water_drip_4.wav");
	m_audioEngine->loadSound("../Audio/water_drip_5.wav");
	m_audioEngine->loadSound("../Audio/water_drip_6.wav");
	m_audioEngine->loadSound("../Audio/water_drip_7.wav");
#pragma endregion
#pragma region DEATHS
	m_audioEngine->loadSound("../Audio/death_enemy.wav");
	m_audioEngine->loadSound("../Audio/death_me_1.wav");
	m_audioEngine->loadSound("../Audio/death_me_2.wav");
	m_audioEngine->loadSound("../Audio/death_me_3.wav");
	m_audioEngine->loadSound("../Audio/death_me_4.wav");
	m_audioEngine->loadSound("../Audio/death_me_5.wav");
	m_audioEngine->loadSound("../Audio/death_me_6.wav");
#pragma endregion
#pragma region MISCELLANEOUS
	m_audioEngine->loadSound("../Audio/guitar.wav");
#pragma endregion
}

void AudioSystem::update(Camera& cam, float dt, float alpha) {
	for (auto e : entities) {
		auto audioC = e->getComponent<AudioComponent>();

		// - - - S O U N D S ---------------------------------------------------------------------------
		{
			int randomSoundIndex = 0;
			int soundPoolSize = 0;
			Audio::SoundInfo_General* soundGeneral;
			Audio::SoundInfo_Unique* soundUnique;

			for (int i = 0; i < Audio::SoundType::COUNT; i++) {
				soundGeneral = &audioC->m_sounds[i];

				soundPoolSize = audioData.m_soundsUnique[i].size();
				if (soundPoolSize > 0) {

					if (soundGeneral->isPlaying) {

						// Starts a new sound from relevant pool of sounds IF NOT ALREADY PLAYING
						if (!soundGeneral->hasStartedPlaying) {

							if (soundPoolSize > 1) {
								randomSoundIndex = rand() % soundPoolSize;
								if (randomSoundIndex == soundGeneral->prevRandomNum) {
									randomSoundIndex++;
									randomSoundIndex = (randomSoundIndex % soundPoolSize);
								}
								soundGeneral->prevRandomNum = randomSoundIndex;
							}
							else {
								randomSoundIndex = 0;
							}

							// To make the code easier to read
							soundUnique = &audioData.m_soundsUnique[i].at(randomSoundIndex);

							soundGeneral->soundID = m_audioEngine->initializeSound(soundUnique->fileName, soundUnique->volume);
							soundGeneral->hasStartedPlaying = true;
							soundGeneral->durationElapsed = 0.0f;
							soundGeneral->currentSoundsLength = soundUnique->soundEffectLength;
						}

						// Start playing the sound if it's not already playing
						if (soundGeneral->durationElapsed == 0.0f) {
							m_audioEngine->startSpecificSound(soundGeneral->soundID);
						}

						// Update the sound with the current positions if it's playing
						if (soundGeneral->durationElapsed < soundGeneral->currentSoundsLength) {
							m_audioEngine->updateSoundWithCurrentPosition(
								soundGeneral->soundID, cam, *e->getComponent<TransformComponent>(),
								soundGeneral->positionalOffset, alpha);

							soundGeneral->durationElapsed += dt;
						}
						else {
							soundGeneral->durationElapsed = 0.0f; // Reset the sound effect to its beginning
							m_audioEngine->stopSpecificSound(soundGeneral->soundID);
							soundGeneral->hasStartedPlaying = false;

							// If the sound isn't looping then make it stop
							if (soundGeneral->playOnce == true) {
								soundGeneral->isPlaying = false;
							}
						}
						// If the sound should no longer be playing stop it and reset its timer
					}
					else if (soundGeneral->durationElapsed != 0.0f) {
						m_audioEngine->stopSpecificSound(soundGeneral->soundID);
						soundGeneral->durationElapsed = 0.0f;
					}
				}
			}
		}

		// - - - S T R E A M I N G  --------------------------------------------------------------------
		{
			// Playing STREAMED sounds
			std::list<std::pair<std::string, Audio::StreamRequestInfo>>::iterator i;
			std::list<std::pair<std::string, Audio::StreamRequestInfo>>::iterator toBeDeleted;
			std::list<std::pair<std::string, std::pair<int, bool>>>::iterator j;
			std::list<std::pair<std::string, std::pair<int, bool>>>::iterator k;
			std::list<std::pair<std::string, std::pair<int, bool>>>::iterator streamToBeDeleted;

			std::string filename = "";
			float volume = 1.0f;
			bool isPositionalAudio;
			bool isLooping;
			int streamIndex = 0;

			for (i = audioC->m_streamingRequests.begin(); i != audioC->m_streamingRequests.end();) {

				if (i->second.startTRUE_stopFALSE == true) {
					// Fetch found stream-request's filename
					filename = i->first;
					volume = i->second.volume;
					isPositionalAudio = i->second.isPositionalAudio;
					isLooping = i->second.isLooping;

					toBeDeleted = i;
					i++;

					streamIndex = m_audioEngine->getAvailableStreamIndex();

					if (streamIndex == -1) {
						Logger::Error("Too many sounds already streaming; failed to stream another one!");
					}
					else {

						Application::getInstance()->pushJobToThreadPool(
							[this, filename, streamIndex, volume, isPositionalAudio, isLooping](int id) {
								return m_audioEngine->streamSound(filename, streamIndex, volume, isPositionalAudio, isLooping);
							});

						audioC->m_currentlyStreaming.emplace_back(filename, std::pair(streamIndex, isPositionalAudio));
						audioC->m_streamingRequests.erase(toBeDeleted);
					}
				}
				else/*if (i.second == false)*/ {

					filename = i->first;
					toBeDeleted = i;
					i++;

					for (j = audioC->m_currentlyStreaming.begin(); j != audioC->m_currentlyStreaming.end();) {

						streamToBeDeleted = j;
						j++;

						if (streamToBeDeleted->first == filename) {

							bool expectedValue = false;
							while (!m_audioEngine->m_streamLocks[streamToBeDeleted->second.first].compare_exchange_strong(expectedValue, true));

							m_audioEngine->stopSpecificStream(streamToBeDeleted->second.first);
							audioC->m_currentlyStreaming.erase(streamToBeDeleted);

							break;
						}
					}
					audioC->m_streamingRequests.erase(toBeDeleted);
				}
			}

			for (k = audioC->m_currentlyStreaming.begin(); k != audioC->m_currentlyStreaming.end();) {
				
				if (k->second.second) {
					m_audioEngine->updateStreamWithCurrentPosition(
						k->second.first, cam, *e->getComponent<TransformComponent>(),
						glm::vec3{ 0.0f, 0.0f, 0.0f }, alpha);
				}

				k++;
			}
		}
	}
}

void AudioSystem::stop() {
	m_audioEngine->stopAllStreams();
}

AudioEngine* AudioSystem::getAudioEngine() {
	return m_audioEngine;
}