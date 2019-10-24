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

					// TODO: Move into receiversystem
					EntityFactory::CreateProjectile(gun->position, gun->direction * gun->projectileSpeed, true);

					// If this is the first shot in this "burst" of projectiles...
					if (!gun->firingContinuously) {
						// ...let the network know.
						e->getComponent<NetworkSenderComponent>()->addDataType(
							Netcode::MessageType::SHOOT_START
						);
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
			}

			gun->firingContinuously = false;
		}

		gun->gunOverloadTimer -= dt;
		gun->projectileSpawnTimer -= dt;
	}
}