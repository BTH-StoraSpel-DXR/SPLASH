#include "pch.h"
#include "MovementPostCollisionSystem.h"
#include "..//..//components/TransformComponent.h"
#include "..//..//components/MovementComponent.h"
#include "..//..//components/CollisionSpheresComponent.h"
#include "..//..//Entity.h"
#include "Sail/utils/GameDataTracker.h"

MovementPostCollisionSystem::MovementPostCollisionSystem() {
	registerComponent<TransformComponent>(true, true, true);
	registerComponent<MovementComponent>(true, true, true);
}

MovementPostCollisionSystem::~MovementPostCollisionSystem() {
}

void MovementPostCollisionSystem::update(float dt) {
	for (auto& e : entities) {
		TransformComponent* transform = e->getComponent<TransformComponent>();
		MovementComponent* movement = e->getComponent<MovementComponent>();

		// Apply air drag
		float saveY = movement->velocity.y;
		movement->velocity.y = 0;
		float vel = glm::length(movement->velocity);

		if (vel > 0.0f) {
			vel = glm::max(vel - movement->airDrag * dt, 0.0f);
			movement->velocity = glm::normalize(movement->velocity) * vel;
		}
		movement->velocity.y = saveY;

		// Update position with velocities after CollisionSystem has potentially altered them
		glm::vec3 translation = (movement->oldVelocity + movement->velocity) * (0.5f * movement->updateableDt);
		if (translation != glm::vec3(0.0f)) {
			transform->translate(translation);
			if (e->getName() == "player") {
				GameDataTracker::getInstance().logDistanceWalked(translation);
			}
		}

		movement->oldVelocity = movement->velocity;

		// Dumb thing for now, will hopefully be done cleaner in the future
		if (CollisionSpheresComponent * csc = e->getComponent<CollisionSpheresComponent>()) {
			csc->spheres[0].position = transform->getTranslation() + glm::vec3(0, 1, 0) * csc->spheres[0].radius;
			csc->spheres[1].position = transform->getTranslation() + glm::vec3(0, 1, 0) * (0.9f * 2.0f - csc->spheres[1].radius);
		}
	}
}
