#include "pch.h"
#include "ProjectileSystem.h"
#include "Sail/entities/components/ProjectileComponent.h"
#include "Sail/entities/components/CandleComponent.h"
#include "Sail/entities/components/CollisionComponent.h"
#include "Sail/entities/components/MovementComponent.h"
#include "Sail/Application.h"

ProjectileSystem::ProjectileSystem() {
	// TODO: System owner should check if this is correct
	registerComponent<ProjectileComponent>(true, true, true);
	registerComponent<CollisionComponent>(true, true, false);
	registerComponent<MovementComponent>(true, true, true);
	registerComponent<CandleComponent>(false, true, true);

	m_splashMinTime = 0.3f;

	float splashSize = 0.2f;
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

			if (collision.entity->hasComponent<CandleComponent>()) {
				// TODO: Consume da waterball (smök)
				collision.entity->getComponent<CandleComponent>()->hitWithWater(e->getComponent<ProjectileComponent>()->m_damage);

				// Queue an instance of this event to the networkSenderSystem
				unsigned __int32 test = e->getComponent<ProjectileComponent>()->ownedBy;

				collision.entity->getComponent<CandleComponent>()->hitByLocalPlayer = true;
			}
		}

		projComp->timeSinceLastDecal += dt;
	}
}