#include "pch.h"

#include "GunSystem.h"

#include "Sail/entities/ECS.h"

#include "Sail/entities/components/GunComponent.h"
#include "Sail/entities/components/NetworkSenderComponent.h"
#include "Sail/utils/GameDataTracker.h"
#include "../Sail/src/Network/NWrapperSingleton.h"
#include "Sail/netcode/NetworkedStructs.h"
#include <random>

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

		if (gun->firing) {
			if (gun->gunOverloadTimer <= 0) {
				if ((gun->gunOverloadvalue += dt) > gun->gunOverloadThreshold) {
					gun->gunOverloadTimer = gun->m_gunOverloadCooldown;
					gun->gunOverloadvalue = 0;
				}

				if (gun->projectileSpawnTimer <= 0.f) {
					gun->projectileSpawnTimer = gun->m_projectileSpawnCooldown;

					m_gameDataTracker->logWeaponFired();
					
					// Tell yours and everybody else's NetworkReceiverSystem to spawn the projectile
					NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
						Netcode::MessageType::SPAWN_PROJECTILE,
						SAIL_NEW Netcode::MessageSpawnProjectile{
							gun->position,
							gun->direction * gun->projectileSpeed,
							e->getComponent<NetworkSenderComponent>()->m_id
						}
					);
				}
			}

			gun->firing = false;
		}
		else {
			if (gun->gunOverloadvalue > 0) {
				gun->gunOverloadvalue -= dt;
			}
		}

		gun->gunOverloadTimer -= dt;
		gun->projectileSpawnTimer -= dt;
	}
}