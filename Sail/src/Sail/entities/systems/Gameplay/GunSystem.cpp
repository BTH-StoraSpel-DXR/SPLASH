#include "pch.h"

#include "GunSystem.h"


#include "Sail/entities/ECS.h"

#include "Sail/entities/components/GunComponent.h"
#include "Sail/entities/components/NetworkSenderComponent.h"
#include "Sail/entities/components/MovementComponent.h"
#include "Sail/entities/components/PowerUpComponent.h"

#include "Sail/utils/GameDataTracker.h"
#include "../Sail/src/Network/NWrapperSingleton.h"
#include "Sail/netcode/NetworkedStructs.h"
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <random>


// TODO: Remove, here only for temporary debugging
#include <iostream>

GunSystem::GunSystem() : BaseComponentSystem() {
	// TODO: System owner should check if this is correct
	registerComponent<GunComponent>(true, true, true);
	registerComponent<MovementComponent>(true, true, false);
	registerComponent<NetworkSenderComponent>(false, true, true);
	registerComponent<NetworkReceiverComponent>(true, true, false);
	registerComponent<PowerUpComponent>(false, true, false);

	m_gameDataTracker = &GameDataTracker::getInstance();
}

GunSystem::~GunSystem() {

}

void GunSystem::setOctree(Octree* octree) {
	m_octree = octree;
}

void GunSystem::update(float dt) {
	for (auto& e : entities) {
		GunComponent* gun = e->getComponent<GunComponent>();
		// Gun is firing and is not overloaded
		if (gun->firing && gun->gunOverloadTimer <= 0) {
			// SHOOT
			if (gun->projectileSpawnTimer <= 0.f) {
					// Determine projectileSpeed based on how long the gun has been firing continuously
				PowerUpComponent* powC = e->getComponent<PowerUpComponent>();
				if (powC) {
					if (powC->powerUps[PowerUpComponent::PowerUps::POWERWASH].time > 0) {
						gun->projectileSpeed = gun->baseProjectileSpeed * 2;
					}
					else {
						alterProjectileSpeed(gun);
					}
				}
				else {
					alterProjectileSpeed(gun);
				}

				m_gameDataTracker->logWeaponFired();
				fireGun(e, gun, powC);
			}
			// DO NOT SHOOT (Cooldown between shots)
			else {
				gun->firingContinuously = false;
			}

			// Overload the gun if necessary
			if ((gun->gunOverloadvalue += dt) > gun->gunOverloadThreshold) {
				overloadGun(e, gun);
			}
		} else { // Gun is not firing.
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

		// Send shooting events based on the GunState
		sendShootingEvents(e);

		gun->gunOverloadTimer -= dt;
		gun->projectileSpawnTimer -= dt;
	}
}

#ifdef DEVELOPMENT
unsigned int GunSystem::getByteSize() const {
	return BaseComponentSystem::getByteSize() + sizeof(*this);
}
#endif

void GunSystem::alterProjectileSpeed(GunComponent* gun) {
	gun->projectileSpeed = gun->baseProjectileSpeed + (gun->projectileSpeedRange * (gun->gunOverloadvalue/gun->gunOverloadThreshold));
}

void GunSystem::fireGun(Entity* e, GunComponent* gun, PowerUpComponent* powC) {
	gun->projectileSpawnTimer = gun->m_projectileSpawnCooldown;

	// If this is the first shot in this "burst" of projectiles...
	if (!gun->firingContinuously) {
		setGunStateSTART(e, gun);
	} else {
		setGunStateLOOP(e, gun);
	}

	gun->firingContinuously = true;

	// Update the frequency of the lowpass based on guns current overload percentage
	float frequency = 9000 - (9000 - 2000) * (gun->gunOverloadvalue / gun->gunOverloadThreshold);
	if (e->hasComponent<AudioComponent>()) {
		e->getComponent<AudioComponent>()->m_sounds[Audio::SHOOT_START].frequency = frequency;
		e->getComponent<AudioComponent>()->m_sounds[Audio::SHOOT_LOOP].frequency = frequency;
		e->getComponent<AudioComponent>()->m_sounds[Audio::SHOOT_END].frequency = frequency;
	}

	Netcode::PlayerID myPlayerID = Netcode::getComponentOwner(e->getComponent<NetworkSenderComponent>()->m_id);

	// Tell yours and everybody else's NetworkReceiverSystem to spawn the projectile
	std::vector<glm::vec3> vel;
	vel.push_back(gun->direction * gun->projectileSpeed + e->getComponent<MovementComponent>()->velocity);
	constexpr float randomSpread = 0.05f;
	if (powC) {
		if (powC->powerUps[PowerUpComponent::PowerUps::SHOWER].time > 0.0f) {
			glm::vec3 right = glm::normalize(glm::cross(gun->direction, glm::vec3(0.0f, 1.0f, 0.0f)));
			glm::vec3 up = glm::normalize(glm::cross(vel.front(), right));

			vel.push_back(glm::normalize(gun->direction + 0.3f*right + 0.3f*up) * gun->projectileSpeed + e->getComponent<MovementComponent>()->velocity);
			vel.push_back(glm::normalize(gun->direction - 0.3f*right + 0.3f*up) * gun->projectileSpeed + e->getComponent<MovementComponent>()->velocity);
			vel.push_back(glm::normalize(gun->direction + 0.3f*right - 0.3f*up ) * gun->projectileSpeed + e->getComponent<MovementComponent>()->velocity);
			vel.push_back(glm::normalize(gun->direction - 0.3f*right - 0.3f*up) * gun->projectileSpeed + e->getComponent<MovementComponent>()->velocity);
		}
		else {
			vel.push_back(gun->direction * gun->projectileSpeed + e->getComponent<MovementComponent>()->velocity);
		}
	}
	else {
		vel.push_back(gun->direction * gun->projectileSpeed + e->getComponent<MovementComponent>()->velocity);
	}

	for (auto& velocity : vel) {

		
		glm::vec3 randPos;
		randPos.r = (Utils::rnd() * randomSpread * 2 - randomSpread);
		randPos.g = (Utils::rnd() * randomSpread * 2 - randomSpread);
		randPos.b = (Utils::rnd() * randomSpread * 2 - randomSpread);

		randPos += glm::normalize(velocity) * (Utils::rnd() * randomSpread * 2 - randomSpread) * 5.0f;
		
		auto rayFrom = e->getComponent<TransformComponent>()->getTranslation();
		rayFrom.y = gun->position.y;
		Octree::RayIntersectionInfo rayInfo;
		auto rayDir = gun->position - rayFrom;
		auto rayDirNorm = glm::normalize(rayDir);

		m_octree->getRayIntersection(rayFrom, rayDirNorm, &rayInfo, e, 0.1f);
		if (!(rayInfo.closestHit < glm::length(rayDir))) {
			// Tell yours and everybody else's NetworkReceiverSystem to spawn the projectile
			NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
				Netcode::MessageType::SPAWN_PROJECTILE,
				SAIL_NEW Netcode::MessageSpawnProjectile{
					gun->position + randPos,
					velocity,
					Netcode::generateUniqueComponentID(myPlayerID), // Generate unique ComponentID here for our own projectiles
					e->getComponent<NetworkSenderComponent>()->m_id,
					frequency
				}
			);
		}
	}
	
}

void GunSystem::overloadGun(Entity* e, GunComponent* gun) {
	// Don't overload the gun at all if the cooldown is '0'
	if (gun->m_gunOverloadCooldown == 0.0f) {
		gun->gunOverloadvalue = gun->gunOverloadThreshold;
		return;
	} 

	gun->gunOverloadvalue = 0;
	gun->gunOverloadTimer = 0;

	// If we have some sort of cooldown, behave statewise as usual.
	setGunStateEND(e, gun);
	gun->firingContinuously = false;	
}

void GunSystem::setGunStateSTART(Entity* e, GunComponent* gun) {
	e->getComponent<NetworkSenderComponent>()->addMessageType(
		Netcode::MessageType::SHOOT_START
	);

	gun->state = GunState::STARTING;
}

void GunSystem::setGunStateLOOP(Entity* e, GunComponent* gun) {
	// Network automatically handles transition from starting-->looping
	gun->state = GunState::LOOPING;
}

void GunSystem::setGunStateEND(Entity* e, GunComponent* gun) {
	e->getComponent<NetworkSenderComponent>()->addMessageType(
		Netcode::MessageType::SHOOT_END
	);
	gun->state = GunState::ENDING;
}

void GunSystem::sendShootingEvents(Entity* e) {
	GunComponent* gun = e->getComponent<GunComponent>();

	switch (gun->state) {
	case GunState::STARTING:
		EventDispatcher::Instance().emit(StartShootingEvent(
			e->getComponent<NetworkReceiverComponent>()->m_id,
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_START].frequency
		));
		break;
	case GunState::LOOPING:
		EventDispatcher::Instance().emit(LoopShootingEvent(
			e->getComponent<NetworkReceiverComponent>()->m_id,
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_LOOP].frequency
		));
		break;
	case GunState::ENDING:
		EventDispatcher::Instance().emit(StopShootingEvent(
			e->getComponent<NetworkReceiverComponent>()->m_id,
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_END].frequency
		));
		gun->state = GunState::STANDBY;
		break;
	default:
		break;
	}
}
