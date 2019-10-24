#include "pch.h"

#include "GunSystem.h"


#include "Sail/entities/ECS.h"

#include "Sail/entities/components/GunComponent.h"
#include "Sail/utils/GameDataTracker.h"
#include "../Sail/src/Network/NWrapperSingleton.h"
#include "Sail/netcode/NetworkedStructs.h"
#include <random>

// TODO: Remove, here only for temporary debugging
#include <iostream>

GunSystem::GunSystem() : BaseComponentSystem() {
	// TODO: System owner should check if this is correct
	registerComponent<GunComponent>(true, true, true);
	m_gameDataTracker = &GameDataTracker::getInstance();
}

GunSystem::~GunSystem() {

}


void GunSystem::update(float dt) {
	for (auto& e : entities) {
		GunComponent* gun = e->getComponent<GunComponent>();

		// If gun is firing
		if (gun->firing) {

			// If gun is not overloaded
			if (gun->gunOverloadTimer <= 0) {

				// SHOOT
				if (gun->projectileSpawnTimer <= 0.f) {
					gun->projectileSpawnTimer = gun->m_projectileSpawnCooldown;

					// Stays here, not in receiver since this is neither per-frame or per-event.
					// it is an event with a duration, something which needs its own definition and space
					// if we decided to implement more of the same type. Until then it should be here.
					// ( Same logic for the sounds being played later on in this update function ) 
					EntityFactory::CreateProjectile(gun->position, gun->direction * gun->projectileSpeed, true);

					// If this is the first shot in this "burst" of projectiles...
					if (!gun->firingContinuously) {
						// ...let the network know.
						e->getComponent<NetworkSenderComponent>()->addDataType(
							Netcode::MessageType::SHOOT_START
						);

						// Audio
						gun->state = GunState::STARTING;
					}
					else {
						gun->state = GunState::LOOPING;
					}

					gun->firingContinuously = true;
					m_gameDataTracker->logWeaponFired();
				}
				// DO NOT SHOOT
				else {
					gun->firingContinuously = false;
				}

				// Overload the gun if necessary
				if ((gun->gunOverloadvalue += dt) > gun->gunOverloadThreshold) {
					gun->gunOverloadTimer = gun->m_gunOverloadCooldown;
					gun->gunOverloadvalue = 0;

					// Let network know we stopped shooting (cus of cooldown).
					std::cout << gun->firing << " | ";
					std::cout << gun->firingContinuously << "\n";
					e->getComponent<NetworkSenderComponent>()->addDataType(
						Netcode::MessageType::SHOOT_END
					);
					gun->state = GunState::ENDING;

					gun->firingContinuously = false;
				}
			}
		}
		else {

			if (gun->gunOverloadvalue > 0) {
				gun->gunOverloadvalue -= dt;
			}

			// If we were firing last frame but now we are not...
			if (gun->firingContinuously) {
				// ...let network know we stopped shooting.
				e->getComponent<NetworkSenderComponent>()->addDataType(
					Netcode::MessageType::SHOOT_END
				);
				gun->state = GunState::ENDING;
			}

			gun->firingContinuously = false;
		}

		// Play sounds depending on which state the gun is in.
		if (gun->state == GunState::STARTING) {
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_START].isPlaying = true;
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_START].playOnce = true;
		}
		else if (gun->state == GunState::LOOPING) {
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_START].isPlaying = false;
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_LOOP].isPlaying = true;
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_LOOP].playOnce = true;
		}
		else if (gun->state == GunState::ENDING) {
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_LOOP].isPlaying = false;
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_END].isPlaying = true;
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_END].playOnce = true;
			
			gun->state = GunState::STANDBY;
		}
		else {	/* gun->state == GunState::STANDBY */

		}

		gun->gunOverloadTimer -= dt;
		gun->projectileSpawnTimer -= dt;
	}
}