#include "pch.h"

#include "AudioSystem.h"

#include "..//Sail/src/API/Audio/AudioEngine.h"
#include "Sail/Application.h"
#include "Sail/entities/systems/Audio/AudioData.h"
#include "Sail/entities/components/AudioComponent.h"
#include "Sail/entities/components/TransformComponent.h"
#include "Sail/entities/components/CandleComponent.h"
#include "Sail/entities/components/LocalOwnerComponent.h"
#include "Sail/entities/Entity.h"
#include "Sail/graphics/camera/Camera.h"
#include "Sail/../Network/NWrapperSingleton.h"

#include "Sail/events/Events.h"

#include <iterator>

#include <mmeapi.h>		// Used to determine if there are outputDevices for sound
#include <windows.h>	//
#include <mmsystem.h>	//

AudioSystem::AudioSystem() : BaseComponentSystem() {
	registerComponent<AudioComponent>(true, true, true);
	registerComponent<TransformComponent>(true, true, false);

	m_audioEngine = SAIL_NEW AudioEngine();

	EventDispatcher::Instance().subscribe(Event::Type::WATER_HIT_PLAYER, this);
	EventDispatcher::Instance().subscribe(Event::Type::PLAYER_DEATH, this);
	EventDispatcher::Instance().subscribe(Event::Type::PLAYER_JUMPED, this);
	EventDispatcher::Instance().subscribe(Event::Type::PLAYER_LANDED, this);
	EventDispatcher::Instance().subscribe(Event::Type::START_SHOOTING, this);
	EventDispatcher::Instance().subscribe(Event::Type::LOOP_SHOOTING, this);
	EventDispatcher::Instance().subscribe(Event::Type::STOP_SHOOTING, this);
	EventDispatcher::Instance().subscribe(Event::Type::CHANGE_WALKING_SOUND, this);
	EventDispatcher::Instance().subscribe(Event::Type::STOP_WALKING, this);
	EventDispatcher::Instance().subscribe(Event::Type::START_THROWING, this);
	EventDispatcher::Instance().subscribe(Event::Type::STOP_THROWING, this);

	initialize();
}

AudioSystem::~AudioSystem() {
	m_audioEngine->stopAllSounds();
	delete m_audioEngine;

	EventDispatcher::Instance().unsubscribe(Event::Type::WATER_HIT_PLAYER, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::PLAYER_DEATH, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::PLAYER_JUMPED, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::PLAYER_LANDED, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::START_SHOOTING, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::STOP_SHOOTING, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::CHANGE_WALKING_SOUND, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::STOP_WALKING, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::START_THROWING, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::STOP_THROWING, this);
}

// TO DO: move to constructor?
void AudioSystem::initialize() {
#pragma region FOOTSTEPS
	m_audioEngine->loadSound("footsteps/footsteps_metal_1.wav");
	m_audioEngine->loadSound("footsteps/footsteps_metal_2.wav");
	m_audioEngine->loadSound("footsteps/footsteps_metal_3.wav");
	m_audioEngine->loadSound("footsteps/footsteps_metal_4.wav");
	m_audioEngine->loadSound("footsteps/footsteps_tile_1.wav");
	m_audioEngine->loadSound("footsteps/footsteps_tile_2.wav");
	m_audioEngine->loadSound("footsteps/footsteps_tile_3.wav");
	m_audioEngine->loadSound("footsteps/footsteps_tile_4.wav");
	m_audioEngine->loadSound("footsteps/footsteps_water_metal_1.wav");
	m_audioEngine->loadSound("footsteps/footsteps_water_metal_2.wav");
	m_audioEngine->loadSound("footsteps/footsteps_water_metal_3.wav");
	m_audioEngine->loadSound("footsteps/footsteps_water_metal_4.wav");
	m_audioEngine->loadSound("footsteps/footsteps_water_tile_1.wav");
	m_audioEngine->loadSound("footsteps/footsteps_water_tile_2.wav");
	m_audioEngine->loadSound("footsteps/footsteps_water_tile_3.wav");
	m_audioEngine->loadSound("footsteps/footsteps_water_tile_4.wav");
#pragma endregion
#pragma region JUMPING
	m_audioEngine->loadSound("jumping/jump.wav");
	m_audioEngine->loadSound("jumping/landing_ground.wav");
#pragma endregion
#pragma region WATERGUN
	m_audioEngine->loadSound("watergun/watergun_start.wav");
	m_audioEngine->loadSound("watergun/watergun_loop.wav");
	m_audioEngine->loadSound("watergun/watergun_end.wav");
	m_audioEngine->loadSound("watergun/watergun_reload.wav");
#pragma endregion
#pragma region SANITY
	m_audioEngine->loadSound("sanity/heart_firstbeat1.wav");
	m_audioEngine->loadSound("sanity/heart_firstbeat2.wav");
	m_audioEngine->loadSound("sanity/heart_firstbeat3.wav");
	m_audioEngine->loadSound("sanity/heart_firstbeat4.wav");
	m_audioEngine->loadSound("sanity/heart_firstbeat5.wav");
	m_audioEngine->loadSound("sanity/heart_firstbeat6.wav");
	m_audioEngine->loadSound("sanity/heart_secondbeat1.wav");
	m_audioEngine->loadSound("sanity/heart_secondbeat2.wav");
	m_audioEngine->loadSound("sanity/heart_secondbeat3.wav");
	m_audioEngine->loadSound("sanity/heart_secondbeat4.wav");
	m_audioEngine->loadSound("sanity/heart_secondbeat5.wav");
	m_audioEngine->loadSound("sanity/heart_secondbeat6.wav");
	m_audioEngine->loadSound("sanity/insanity_scream.wav");
	m_audioEngine->loadSound("sanity/insanity_violin1.wav");
	m_audioEngine->loadSound("sanity/insanity_violin2.wav");
	m_audioEngine->loadSound("sanity/insanity_violin3.wav");
	m_audioEngine->loadSound("sanity/insanity_violin4.wav");
#pragma endregion
#pragma region IMPACTS
	m_audioEngine->loadSound("impacts/water_impact_enemy_candle.wav");
	m_audioEngine->loadSound("impacts/water_impact_my_candle.wav");
	m_audioEngine->loadSound("impacts/water_drip_1.wav");
	m_audioEngine->loadSound("impacts/water_drip_2.wav");
	m_audioEngine->loadSound("impacts/water_drip_3.wav");
	m_audioEngine->loadSound("impacts/water_drip_4.wav");
	m_audioEngine->loadSound("impacts/water_drip_5.wav");
	m_audioEngine->loadSound("impacts/water_drip_6.wav");
	m_audioEngine->loadSound("impacts/water_drip_7.wav");
#pragma endregion
#pragma region DEATHS
	m_audioEngine->loadSound("death/killing_blow.wav");
	m_audioEngine->loadSound("death/death_1.wav");
	m_audioEngine->loadSound("death/death_2.wav");
	m_audioEngine->loadSound("death/death_3.wav");
	m_audioEngine->loadSound("death/death_4.wav");
	m_audioEngine->loadSound("death/death_5.wav");
	m_audioEngine->loadSound("death/death_6.wav");
#pragma endregion
#pragma region MISCELLANEOUS
	m_audioEngine->loadSound("miscellaneous/re_ignition_candle.wav");
	// Throwing
	m_audioEngine->loadSound("miscellaneous/throwing/start_throw.wav");
	for (int i = 1; i < 8; i++) {
		m_audioEngine->loadSound("miscellaneous/throwing/throw" + std::to_string(i) + ".wav");
	}
  // Sprinkler
	m_audioEngine->loadSound("miscellaneous/sprinkler_start1.wav");
	m_audioEngine->loadSound("miscellaneous/sprinkler_start2.wav");
	m_audioEngine->loadSound("miscellaneous/sprinkler.wav");
	m_audioEngine->loadSound("miscellaneous/alarm.wav");
#pragma endregion
}

void AudioSystem::update(Camera& cam, float dt, float alpha) {
	float tempVolumeLevel = Application::getInstance()->getSettings().applicationSettingsDynamic["sound"]["global"].value;

	if (waveOutGetNumDevs() == 0) {
		m_hasOutputDevices = false;
	}
	if (!m_hasOutputDevices) {	// Only run audiosystem if there are outputdevices on the computer
		return;
	}


	
	for (auto e : entities) {
		auto audioC = e->getComponent<AudioComponent>();

		// - - - S O U N D S ---------------------------------------------------------------------------
		{
			int randomSoundIndex = 0;
			int soundPoolSize = 0;
			Audio::SoundInfo_General* soundGeneral;
			Audio::SoundInfo_Unique* soundUnique;

			
			for (int soundTypeIndex = 0; soundTypeIndex < Audio::SoundType::COUNT; soundTypeIndex++) {
				soundGeneral = &audioC->m_sounds[soundTypeIndex];

				// Deal with all sounds & streams except death sound
				if (soundTypeIndex == Audio::SoundType::DEATH || soundTypeIndex == Audio::SoundType::INSANITY_SCREAM) {
					continue;
				}

				soundPoolSize = audioData.m_soundsUnique[soundTypeIndex].size();
				if (soundPoolSize > 0) {
					
					if (soundGeneral->isPlaying) {

						// Starts a new sound from relevant pool of sounds IF NOT ALREADY PLAYING
						if (!soundGeneral->hasStartedPlaying) {

							randomSoundIndex = randomASoundIndex(soundPoolSize, soundGeneral);

							soundUnique = &audioData.m_soundsUnique[soundTypeIndex].at(randomSoundIndex);
							soundGeneral->volume = (soundUnique->volume * tempVolumeLevel);
							soundGeneral->hasStartedPlaying = true;
							soundGeneral->durationElapsed = 0.0f;
							soundGeneral->currentSoundsLength = soundUnique->soundEffectLength;

				
							// To make the code easier to read
							soundGeneral->soundID = m_audioEngine->beginSound(
								soundUnique->fileName,
								soundGeneral->effect,
								soundGeneral->frequency,
								(soundGeneral->volume * tempVolumeLevel)
							);
							m_audioEngine->startSpecificSound(soundGeneral->soundID, (soundUnique->volume * tempVolumeLevel));
						}

						// Update the sound with the current positions if it's playing
						if (soundGeneral->durationElapsed < soundGeneral->currentSoundsLength) {
							m_audioEngine->updateSoundWithCurrentPosition(
								soundGeneral->soundID, cam, *e->getComponent<TransformComponent>(),
								soundGeneral->positionalOffset, alpha
							);
							m_audioEngine->setSoundVolume(soundGeneral->soundID, (soundGeneral->volume * tempVolumeLevel));
							
							if (soundGeneral->effect == Audio::EffectType::PROJECTILE_LOWPASS) {
								updateProjectileLowPass(soundGeneral);
							}

							soundGeneral->durationElapsed += dt;
						} else {
							soundGeneral->durationElapsed = 0.0f; // Reset the sound effect to its beginning
							
							soundGeneral->hasStartedPlaying = false;

							soundGeneral->isPlaying = !soundGeneral->playOnce;
						}
					} 
					else if (soundGeneral->hasStartedPlaying) {
						
						soundGeneral->hasStartedPlaying = false;
						soundGeneral->durationElapsed = 0.0f;
					}
				}
			}

			// Deal with death sound (Fixes LOUD deathsound issue)
			dealWithDeathSound(audioC, dt);
			dealwithInsanitySound(audioC, dt);
		}

		// - - - S T R E A M I N G  --------------------------------------------------------------------
		{
			// Deal with requests
			if (audioC->m_streamingRequests.size() > 0)
			for (m_i = audioC->m_streamingRequests.begin(); m_i != audioC->m_streamingRequests.end();) {
				// If the request wants to start
				if (m_i->second.startTRUE_stopFALSE == true) {
					startPlayingRequestedStream(e, audioC);
				} else { // If the request wants to stop
					stopPlayingRequestedStream(e, audioC);
				}
			}

			// Hot fix for lab ambiance not playing after pause.
			//hotFixAmbiance(e, audioC);	// Commented-out because it causes a crash during death of other player

			// Per currently streaming sound
			for (m_k = audioC->m_currentlyStreaming.begin(); m_k != audioC->m_currentlyStreaming.end();) {
				// Update its position in the world
				if (m_k->second.isPositionalAudio) {
					updateStreamPosition(e, cam, alpha);
				}
				// Update volume if it has changed
				if (m_k->second.prevVolume != m_k->second.volume) {
					m_k->second.prevVolume = m_k->second.volume;
					updateStreamVolume();
				}

				m_k++;
			}
		}
	}

#ifdef DEVELOPMENT
	/* Uncomment it if you want to try to fix something audiowise. */
	//m_audioEngine->logDebugData();
#endif
}

void AudioSystem::stop() {
	m_audioEngine->pause_unpause_AllStreams(false);
	m_audioEngine->stopAllStreams();
	m_audioEngine->pauseAllSounds();

	for (auto e : entities) {
		auto audioC = e->getComponent<AudioComponent>();

		audioC->m_currentlyStreaming.clear();
		audioC->m_streamingRequests.clear();
	}
}
int AudioSystem::randomASoundIndex(int soundPoolSize, Audio::SoundInfo_General* soundGeneral) {
	int randomSoundIndex = -1;

	if (soundPoolSize > 1) {
		randomSoundIndex = rand() % soundPoolSize;
		if (randomSoundIndex == soundGeneral->prevRandomNum) {
			randomSoundIndex++;
			randomSoundIndex = (randomSoundIndex % soundPoolSize);
		}
		soundGeneral->prevRandomNum = randomSoundIndex;
	} else {
		randomSoundIndex = 0;
	}

	return randomSoundIndex;
}

void AudioSystem::startPlayingRequestedStream(Entity* e, AudioComponent* audioC) {

	std::string filename = m_i->first;
	float volume = m_i->second.volume;
	bool isPositionalAudio = m_i->second.isPositionalAudio;
	bool isLooping = m_i->second.isLooping;
	int streamIndex = m_audioEngine->getAvailableStreamIndex();
	// Storing this for later use
	m_i->second.streamIndex = streamIndex;

	m_toBeDeleted = m_i;
	m_i++;

	if (streamIndex == -1) {
		SAIL_LOG_ERROR("Too many sounds already streaming; failed to stream another one!");
	} else {
		Application::getInstance()->pushJobToThreadPool(
			[this, filename, streamIndex, volume, isPositionalAudio, isLooping, audioC](int id) {
				return m_audioEngine->streamSound(filename, streamIndex, volume, isPositionalAudio, isLooping, audioC);
			});

		audioC->m_currentlyStreaming.emplace_back(filename, m_toBeDeleted->second);
		audioC->m_streamingRequests.erase(m_toBeDeleted);
	}
}

void AudioSystem::stopPlayingRequestedStream(Entity* e, AudioComponent* audioC) {
	std::string filename = m_i->first;
	m_toBeDeleted = m_i;
	m_i++;

	for (m_j = audioC->m_currentlyStreaming.begin(); m_j != audioC->m_currentlyStreaming.end();) {

		m_streamToBeDeleted = m_j;
		m_j++;

		if (m_streamToBeDeleted->first == filename) {

			bool expectedValue = false;
			while (!m_audioEngine->m_streamLocks[m_streamToBeDeleted->second.streamIndex].compare_exchange_strong(expectedValue, true));

			m_audioEngine->stopSpecificStream(m_streamToBeDeleted->second.streamIndex);
			audioC->m_currentlyStreaming.erase(m_streamToBeDeleted);

			break;
		}
	}
	audioC->m_streamingRequests.erase(m_toBeDeleted);
}

void AudioSystem::updateStreamPosition(Entity* e, Camera& cam, float alpha) {
	if (m_k->second.isPositionalAudio) {
		m_audioEngine->updateStreamWithCurrentPosition(
			m_k->second.streamIndex, cam, *e->getComponent<TransformComponent>(),
			glm::vec3{ 0.0f, 0.0f, 0.0f }, alpha);
	}
}

void AudioSystem::updateStreamVolume() {
	m_audioEngine->setStreamVolume(m_k->second.streamIndex, (m_k->second.volume * Application::getInstance()->getSettings().applicationSettingsDynamic["sound"]["global"].value));
}

void AudioSystem::updateProjectileLowPass(Audio::SoundInfo_General* general) {
	m_audioEngine->updateProjectileLowPass(general->frequency, general->soundID);
}

void AudioSystem::dealWithDeathSound(AudioComponent* audioC, float dt) {
	float tempVolumeLevel = Application::getInstance()->getSettings().applicationSettingsDynamic["sound"]["global"].value;

	Audio::SoundInfo_General* soundGeneral = &audioC->m_sounds[Audio::SoundType::DEATH];
	if (soundGeneral->isPlaying) {
		int soundPoolSize = audioData.m_soundsUnique[Audio::SoundType::DEATH].size();

		if (soundPoolSize > 0) {
			if (!soundGeneral->hasStartedPlaying) {

				int randomSoundIndex = randomASoundIndex(
					soundPoolSize,
					soundGeneral
				);

				Audio::SoundInfo_Unique* soundUnique = &audioData.m_soundsUnique[Audio::SoundType::DEATH].at(randomSoundIndex);
				soundGeneral->volume = (soundUnique->volume * tempVolumeLevel);
				soundGeneral->hasStartedPlaying = true;
				soundGeneral->durationElapsed = 0.0f;
				soundGeneral->currentSoundsLength = soundUnique->soundEffectLength;

				m_audioEngine->startDeathSound(
					soundUnique->fileName,
					(soundGeneral->volume * tempVolumeLevel)
				);
				soundGeneral->playOnce = true;
			}
			// Update the sound with the current positions if it's playing.
			if (soundGeneral->durationElapsed < soundGeneral->currentSoundsLength) {

				m_audioEngine->updateDeathvolume(soundGeneral->volume * tempVolumeLevel);
				soundGeneral->durationElapsed += dt;
			}
			else {
				soundGeneral->durationElapsed = 0.0f; // Reset the sound effect to its beginning

				soundGeneral->hasStartedPlaying = false;

				soundGeneral->isPlaying = !soundGeneral->playOnce;
			}
		}
	}
	else if (soundGeneral->hasStartedPlaying) {
		soundGeneral->hasStartedPlaying = false;
		soundGeneral->durationElapsed = 0.0f;
	}
}

void AudioSystem::dealwithInsanitySound(AudioComponent* audioC, float dt) {
	float tempVolumeLevel = Application::getInstance()->getSettings().applicationSettingsDynamic["sound"]["global"].value;

	Audio::SoundInfo_General* soundGeneral = &audioC->m_sounds[Audio::SoundType::INSANITY_SCREAM];
	if (soundGeneral->isPlaying) {
		int soundPoolSize = audioData.m_soundsUnique[Audio::SoundType::INSANITY_SCREAM].size();

		if (soundPoolSize > 0) {
			if (!soundGeneral->hasStartedPlaying) {

				int randomSoundIndex = randomASoundIndex(
					soundPoolSize,
					soundGeneral
				);

				Audio::SoundInfo_Unique* soundUnique = &audioData.m_soundsUnique[Audio::SoundType::INSANITY_SCREAM].at(randomSoundIndex);
				soundGeneral->volume = (soundUnique->volume * tempVolumeLevel);
				soundGeneral->hasStartedPlaying = true;
				soundGeneral->durationElapsed = 0.0f;
				soundGeneral->currentSoundsLength = soundUnique->soundEffectLength;

				m_audioEngine->startInsanitySound(
					soundUnique->fileName,
					(soundGeneral->volume * tempVolumeLevel)
				);
				soundGeneral->playOnce = true;
			}

			// Update the sound with the current positions if it's playing
			if (soundGeneral->durationElapsed < soundGeneral->currentSoundsLength) {

				m_audioEngine->updateInsanityVolume(soundGeneral->volume * tempVolumeLevel);
				soundGeneral->durationElapsed += dt;
			}
			else {
				soundGeneral->durationElapsed = 0.0f; // Reset the sound effect to its beginning

				soundGeneral->hasStartedPlaying = false;

				soundGeneral->isPlaying = !soundGeneral->playOnce;
			}
		}
	}
	else if (soundGeneral->hasStartedPlaying) {
		soundGeneral->hasStartedPlaying = false;
		soundGeneral->durationElapsed = 0.0f;
	}
}

void AudioSystem::hotFixAmbiance(Entity* e, AudioComponent* audioC) {
	if (e->getName() == "LabAmbiance") {
		// If it's not playing anything...
		if (audioC->m_currentlyStreaming.size() == 0) {
			// ... start playing ambiance again.
			audioC->streamSoundRequest_HELPERFUNC("res/sounds/ambient/ambiance_lab.xwb", true, 1.0f, false, true);
		}
	}
}

AudioEngine* AudioSystem::getAudioEngine() {
	return m_audioEngine;
}

bool AudioSystem::onEvent(const Event& event) {
	auto findFromID = [=](const Netcode::ComponentID netCompID) {
		Entity* source = nullptr;
		for (auto entity : entities) {
			if (auto recComp = entity->getComponent<NetworkReceiverComponent>(); recComp) {
				if (recComp->m_id == netCompID) {
					source = entity;
					break;
				}
			}
		}
		return source;
	};
	
	auto onWaterHitPlayer = [=](const WaterHitPlayerEvent& e) {
		Entity* player = nullptr;
		Entity* candle = nullptr;

		// Find the entity with the correct ID
		if (auto entity = findFromID(e.netCompID); entity) {
			// Find the candle child of that entity
			for (auto child : entity->getChildEntities()) {
				if (child->hasComponent<CandleComponent>()) {
					player = entity;
					candle = child;
					break;
				}
			}
		}

		if (!candle) {
			SAIL_LOG_WARNING("AudioSystem::onWaterHitPlayer: no matching entity found");
			return;
		}
		
		// Play relevant sound if candle is hit
		if (candle->getComponent<CandleComponent>()->isLit) {
			// Check if my candle or other candle
			const auto soundIndex = (player->hasComponent<LocalOwnerComponent>()
				? Audio::SoundType::WATER_IMPACT_MY_CANDLE 
				: Audio::SoundType::WATER_IMPACT_ENEMY_CANDLE);

			player->getComponent<AudioComponent>()->m_sounds[soundIndex].isPlaying = true;
			player->getComponent<AudioComponent>()->m_sounds[soundIndex].playOnce = true;
		}
	};

	auto onPlayerDied = [](const PlayerDiedEvent& e) {
		// Play kill sound if the player was the one who shot
		if (Netcode::getComponentOwner(e.killerID) == NWrapperSingleton::getInstance().getMyPlayerID()) {
			if (auto* audioComp = e.myPlayer->getComponent<AudioComponent>()) {
				auto& killSound = audioComp->m_sounds[Audio::SoundType::KILLING_BLOW];
				killSound.isPlaying = true;
				killSound.playOnce = true;
			}
		}

		//TODO: Find out why death sound is high as fuck!
		else if (e.killerID == Netcode::INSANITY_COMP_ID) {
			if (auto* audioComp = e.myPlayer->getComponent<AudioComponent>()) {
				auto& insanitySound = audioComp->m_sounds[Audio::SoundType::INSANITY_SCREAM];
				insanitySound.isPlaying = true;
				insanitySound.playOnce = true;
			}
		} else {
			// Play death sound
			if (auto* audioComp = e.killed->getComponent<AudioComponent>()) {
				auto& deathSound = audioComp->m_sounds[Audio::SoundType::DEATH];
				deathSound.isPlaying = true;
				deathSound.playOnce = true;
			}
			
		}
	};

	auto onPlayerJumped = [=](const PlayerJumpedEvent& e) {
		if (auto player = findFromID(e.netCompID); player) {
			player->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::JUMP].playOnce = true;
			player->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::JUMP].isPlaying = true;
		} else {
			SAIL_LOG_WARNING("AudioSystem : player jumped but no matching entity found");
		}
	};

	auto onPlayerLanded = [=](const PlayerLandedEvent& e) {
		if (auto player = findFromID(e.netCompID); player) {
			player->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::LANDING_GROUND].playOnce = true;
			player->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::LANDING_GROUND].isPlaying = true;
		} else {
			SAIL_LOG_WARNING("AudioSystem : player landed but no matching entity found");
		}
	};

	auto onStartShooting = [=](const StartShootingEvent& e) {
		if (auto player = findFromID(e.netCompID); player) {
			Audio::SoundInfo_General* soundGeneralStart = &player->getComponent<AudioComponent>()->m_sounds[
				Audio::SoundType::SHOOT_START
			];
			soundGeneralStart->playOnce = true;
			soundGeneralStart->isPlaying = true;
			soundGeneralStart->frequency = e.lowPassFrequency;
		} else {
			SAIL_LOG_WARNING("AudioSystem : started shooting but no matching entity found");
		}
	};

	auto onLoopShooting = [=](const LoopShootingEvent& e) {
		if (auto player = findFromID(e.netCompID); player) {
			Audio::SoundInfo_General* soundGeneralLoop = &player->getComponent<AudioComponent>()->m_sounds[
				Audio::SoundType::SHOOT_LOOP
			];
			player->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_START].isPlaying = false;
			soundGeneralLoop->isPlaying = true;
			soundGeneralLoop->playOnce = true;
			soundGeneralLoop->frequency = e.lowPassFrequency;
		}
		else {
			SAIL_LOG_WARNING("AudioSystem : looped shooting but no matching entity found");
		}
	};

	auto onStopShooting = [=](const StopShootingEvent& e) {
		if (auto player = findFromID(e.netCompID); player) {
			// Force stop the loop sound since it continues playing when it should not.
			Audio::SoundInfo_General* soundGeneralLoop = &player->getComponent<AudioComponent>()->m_sounds[
				Audio::SoundType::SHOOT_LOOP
			];
			m_audioEngine->stopSpecificSound(soundGeneralLoop->soundID);
			Audio::SoundInfo_General* soundGeneralStart = &player->getComponent<AudioComponent>()->m_sounds[
				Audio::SoundType::SHOOT_START
			];
			m_audioEngine->stopSpecificSound(soundGeneralStart->soundID);
			Audio::SoundInfo_General* soundGeneralEnd = &player->getComponent<AudioComponent>()->m_sounds[
				Audio::SoundType::SHOOT_END
			];
			soundGeneralEnd->isPlaying = false;
			soundGeneralEnd->playOnce = true;
			soundGeneralEnd->isPlaying = true;
			soundGeneralEnd->frequency = e.lowPassFrequency;
		} else {
			SAIL_LOG_WARNING("AudioSystem : stopped shooting but no matching entity found");
		}
	};

	auto onChangeWalkingSound = [=](const ChangeWalkingSoundEvent& e) {
		if (auto player = findFromID(e.netCompID); player) {
			// Disable all walking sounds
			player->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::RUN_METAL].isPlaying = false;
			player->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::RUN_TILE].isPlaying = false;
			player->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::RUN_WATER_METAL].isPlaying = false;
			player->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::RUN_WATER_TILE].isPlaying = false;

			// Play the correct walking sound
			player->getComponent<AudioComponent>()->m_sounds[e.soundType].playOnce = false;
			player->getComponent<AudioComponent>()->m_sounds[e.soundType].isPlaying = true;
		} else {
			SAIL_LOG_WARNING("AudioSystem : changed walking sound but no matching entity found");
		}
	};

	auto onStopWalking = [=](const StopWalkingEvent& e) {
		if (auto player = findFromID(e.netCompID); player) {
			player->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::RUN_METAL].isPlaying = false;
			player->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::RUN_TILE].isPlaying = false;
			player->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::RUN_WATER_METAL].isPlaying = false;
			player->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::RUN_WATER_TILE].isPlaying = false;
		} else {
			SAIL_LOG_WARNING("AudioSystem : stopped walking but no matching entity found");
		}
	};

	auto onStartThrowing = [=] (const StartThrowingEvent& e) {
		if (auto player = findFromID(e.netCompID); player) {
			player->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::START_THROWING].playOnce = true;
			player->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::START_THROWING].isPlaying = true;
		} else {
			SAIL_LOG_WARNING("AudioSystem : started throwing but no matching entity found");
		}
	};

	auto onStopThrowing = [=] (const StopThrowingEvent& e) {
		if (auto player = findFromID(e.netCompID); player) {
			player->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::STOP_THROWING].playOnce = true;
			player->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::STOP_THROWING].isPlaying = true;
		} else {
			SAIL_LOG_WARNING("AudioSystem : stopped throwing but no matching entity found");
		}
	};

	switch (event.type) {
	case Event::Type::WATER_HIT_PLAYER: onWaterHitPlayer((const WaterHitPlayerEvent&)event); break;
	case Event::Type::PLAYER_DEATH: 
		for (auto& entity : entities) {
			AudioComponent* ac = entity->getComponent<AudioComponent>();
			for (auto& sound : ac->m_sounds) {
				sound.isPlaying = false;
			}
			// Stop all streams EXCEPT for 'lab ambiance'
			auto& iterator = ac->m_currentlyStreaming.begin();
			while (iterator != ac->m_currentlyStreaming.end()) {
				if ((*iterator).first != "res/sounds/ambient/ambiance_lab.xwb") {
					ac->streamSoundRequest_HELPERFUNC((*iterator).first, false, 1.0f, false, false);
				}
				iterator++;
			}
		}
		onPlayerDied((const PlayerDiedEvent&)event); 
		break;
	case Event::Type::PLAYER_JUMPED: onPlayerJumped((const PlayerJumpedEvent&)event); break;
	case Event::Type::PLAYER_LANDED: onPlayerLanded((const PlayerLandedEvent&)event); break;
	case Event::Type::START_SHOOTING: onStartShooting((const StartShootingEvent&)event); break;
	case Event::Type::LOOP_SHOOTING: onLoopShooting((const LoopShootingEvent&)event); break;
	case Event::Type::STOP_SHOOTING: onStopShooting((const StopShootingEvent&)event); break;
	case Event::Type::CHANGE_WALKING_SOUND: onChangeWalkingSound((const ChangeWalkingSoundEvent&)event); break;
	case Event::Type::STOP_WALKING: onStopWalking((const StopWalkingEvent&)event); break;
	case Event::Type::START_THROWING: onStartThrowing((const StartThrowingEvent&)event); break;
	case Event::Type::STOP_THROWING: onStopThrowing((const StopThrowingEvent&)event); break;
	default: break;
	}

	return true;
}

#ifdef DEVELOPMENT
unsigned int AudioSystem::getByteSize() const {
	unsigned int size = BaseComponentSystem::getByteSize() + sizeof(*this);
	size += m_audioEngine->getByteSize();
	return size;
}
#endif
