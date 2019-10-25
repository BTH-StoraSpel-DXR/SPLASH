#include "pch.h"

#include "GunSystem.h"


#include "Sail/entities/ECS.h"

#include "Sail/entities/components/GunComponent.h"
#include "Sail/entities/components/NetworkSenderComponent.h"
#include "Sail/utils/GameDataTracker.h"
#include "../Sail/src/Network/NWrapperSingleton.h"
#include "Sail/netcode/NetworkedStructs.h"
#include <random>

// TODO: Remove, here only for temporary debugging
#include <iostream>

GunSystem::GunSystem() : BaseComponentSystem() {
	// TODO: System owner should check if this is correct
	registerComponent<GunComponent>(true, true, true);
	registerComponent<NetworkSenderComponent>(false, true, true);
	m_gameDataTracker = &GameDataTracker::getInstance();
}

GunSystem::~GunSystem() {

}


void GunSystem::update(float dt) {
	for (auto& e : entities) {
		GunComponent* gun = e->getComponent<GunComponent>();

		// Gun is firing
		if (gun->firing) {

			// Gun is not overloaded
			if (gun->gunOverloadTimer <= 0) {

				// SHOOT
				if (gun->projectileSpawnTimer <= 0.f) {
					fireGun(e, gun);
				}
				// DO NOT SHOOT (Cooldown between shots)
				else {
					gun->firingContinuously = false;
				}

				// Overload the gun if necessary
				if ((gun->gunOverloadvalue += dt) > gun->gunOverloadThreshold) {
					overloadGun(e, gun);
				}
			}
		}
		// Gun is not firing.
		else {

			// Reduce the overload value
			if (gun->gunOverloadvalue > 0) {
				gun->gunOverloadvalue -= dt;
			}

			// Gun stopped firing
			if (gun->firingContinuously) {
				// Set gun state to END
				setGunStateEND(e, gun);
			}

			gun->firingContinuously = false;
		}

		// Play shooting sounds based on the GunState
		playShootingSounds(e);

		gun->gunOverloadTimer -= dt;
		gun->projectileSpawnTimer -= dt;
	}
}


void GunSystem::fireGun(Entity* e, GunComponent* gun) {
	gun->projectileSpawnTimer = gun->m_projectileSpawnCooldown;

	// Stays here, not in receiver since this is neither per-frame or per-event.
	// it is an event with a duration, something which needs its own definition and space
	// if we decided to implement more of the same type. Until then it should be here.
	// ( Same logic for the sounds being played later on in this update function ) 
	EntityFactory::CreateProjectile(gun->position, gun->direction * gun->projectileSpeed, true);

	// If this is the first shot in this "burst" of projectiles...
	if (!gun->firingContinuously) {
		setGunStateSTART(e, gun);
	}
	else {
		setGunStateLOOP(e, gun);
	}

	gun->firingContinuously = true;
	m_gameDataTracker->logWeaponFired();
}

void GunSystem::overloadGun(Entity* e, GunComponent* gun) {
	gun->gunOverloadTimer = gun->m_gunOverloadCooldown;
	gun->gunOverloadvalue = 0;

	setGunStateEND(e, gun);

	gun->firingContinuously = false;
}

void GunSystem::setGunStateSTART(Entity* e, GunComponent* gun) {
	e->getComponent<NetworkSenderComponent>()->addDataType(
		Netcode::MessageType::SHOOT_START
	);

	gun->state = GunState::STARTING;
}

void GunSystem::setGunStateLOOP(Entity* e, GunComponent* gun) {
	// Network automatically handles transition from starting-->looping
	gun->state = GunState::LOOPING;
}

void GunSystem::setGunStateEND(Entity* e, GunComponent* gun) {
	e->getComponent<NetworkSenderComponent>()->addDataType(
		Netcode::MessageType::SHOOT_END
	);
	gun->state = GunState::ENDING;
}

void GunSystem::playShootingSounds(Entity* e) {
	GunComponent* gun = e->getComponent<GunComponent>();

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
}
