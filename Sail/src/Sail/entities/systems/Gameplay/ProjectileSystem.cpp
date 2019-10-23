#include "pch.h"
#include "ProjectileSystem.h"
#include "Sail/entities/components/ProjectileComponent.h"
#include "Sail/entities/components/CandleComponent.h"
#include "Sail/entities/components/CollisionComponent.h"
#include "Sail/entities/components/MovementComponent.h"
#include "Sail/entities/components/LocalOwnerComponent.h"
#include "Network/NWrapperSingleton.h"
#include "Sail/Application.h"

ProjectileSystem::ProjectileSystem() {
	// TODO: System owner should check if this is correct
	registerComponent<ProjectileComponent>(true, true, true);
	registerComponent<CollisionComponent>(true, true, false);
	registerComponent<MovementComponent>(true, true, true);
	registerComponent<CandleComponent>(false, true, true);
	registerComponent<NetworkReceiverComponent>(false, true, false);
	registerComponent<LocalOwnerComponent>(false, true, false);

	m_splashMinTime = 0.3f;

	float splashSize = 0.14f;
	m_projectileSplashSize = (1.f / splashSize) / 2.f;
}

ProjectileSystem::~ProjectileSystem() {

}

void ProjectileSystem::update(float dt) {
	for (auto& e : entities) {
		CollisionComponent* collisionComp = e->getComponent<CollisionComponent>();
		auto projectileCollisions = collisionComp->collisions;
		auto projComp = e->getComponent<ProjectileComponent>();
		for (auto& collision : projectileCollisions) {
			// Check if a decal should be created
			if (projComp->timeSinceLastDecal > m_splashMinTime &&
				glm::length(e->getComponent<MovementComponent>()->oldVelocity) > 0.7f) {
				// TODO: Replace with some "layer-id" check rather than doing a string check
				if (collision.entity->getName().substr(0U, 4U) == "Map_") {

					glm::mat4 rotMat = glm::rotate(glm::identity<glm::mat4>(), Utils::fastrand() * 3.14f, glm::vec3(0.0f, 0.0f, 1.0f));

					Application::getInstance()->getRenderWrapper()->getCurrentRenderer()->submitDecal(
						collision.intersectionPosition,
						rotMat,
						glm::vec3(m_projectileSplashSize));

					projComp->timeSinceLastDecal = 0.f;
				}
			}

			//If projectile collided with a candle
			if (collision.entity->hasComponent<CandleComponent>()) {
				//If local player owned the projectile
				if (e->hasComponent<LocalOwnerComponent>()) {
					//Inform the host about the hit.( in case you are host this will broadcast to everyone else)
					NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
						Netcode::MessageType::WATER_HIT_PLAYER,
						SAIL_NEW Netcode::MessageDataWaterHitPlayer{
							collision.entity->getParent()->getComponent<NetworkReceiverComponent>()->m_id
						}
					);

					if (NWrapperSingleton::getInstance().isHost()) {
						auto e = collision.entity->getComponent<CandleComponent>();
						e->hitWithWater(10.0f, e->getWasHitByNetID());
					}

					//Check in NetworkReceiverSystem what happens next
				}
			}
		}

		projComp->timeSinceLastDecal += dt;
	}
}