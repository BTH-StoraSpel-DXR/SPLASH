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

		if (gun->firing) {
			if (gun->gunOverloadTimer <= 0) {
				if ((gun->gunOverloadvalue += dt) > gun->gunOverloadThreshold) {
					gun->gunOverloadTimer = gun->m_gunOverloadCooldown;
					gun->gunOverloadvalue = 0;
					gun->m_wasFiringLastFrame = false;

					// Let network know we stopped shooting (cus of cooldown).
					std::cout << "Gunsystem stop Cooldown\n";
					e->getComponent<NetworkSenderComponent>()->addDataType(
						Netcode::MessageType::SHOOT_END
					);
				}

				if (gun->projectileSpawnTimer <= 0.f) {
					gun->projectileSpawnTimer = gun->m_projectileSpawnCooldown;

					// TODO: Move into receiversystem(viktor?)
					EntityFactory::CreateProjectile(gun->position, gun->direction * gun->projectileSpeed, true);

					// If this is the first shot in this "burst" of projectiles...
					if (!gun->m_wasFiringLastFrame) {
						// ...let the network know.
						std::cout << "Gunsystem start fire\n";
						e->getComponent<NetworkSenderComponent>()->addDataType(
							Netcode::MessageType::SHOOT_START
						);
					}

					m_gameDataTracker->logWeaponFired();
				}
			}

			//gun->firing = false;
			gun->m_wasFiringLastFrame = true;
		}
		else {

			if (gun->gunOverloadvalue > 0) {
				gun->gunOverloadvalue -= dt;

				// If we were firing last frame but now we are not...
				if (gun->m_wasFiringLastFrame) {
					gun->m_wasFiringLastFrame = false;

					// ...let network know we stopped shooting.
					std::cout << "Gunsystem stop fire\n";
					e->getComponent<NetworkSenderComponent>()->addDataType(
						Netcode::MessageType::SHOOT_END
					);
				}
			}


		}

		gun->gunOverloadTimer -= dt;
		gun->projectileSpawnTimer -= dt;
	}
}