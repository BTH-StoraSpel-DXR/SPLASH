#include "pch.h"
#include "MovementSystem.h"
#include "Sail/entities/components/TransformComponent.h"
#include "Sail/entities/components/MovementComponent.h"
#include "Sail/entities/components/RagdollComponent.h"
#include "Sail/entities/components/RenderInActiveGameComponent.h"
#include "Sail/entities/components/RenderInReplayComponent.h"
#include "Sail/entities/Entity.h"


template <typename T>
MovementSystem<T>::MovementSystem() {
	registerComponent<TransformComponent>(true, true, true);
	registerComponent<MovementComponent>(true, true, true);
	registerComponent<RagdollComponent>(false, true, false);
	registerComponent<T>(true, false, false);
}

template <typename T>
void MovementSystem<T>::update(float dt) {
	for (auto& e : entities) {

		TransformComponent* transform = e->getComponent<TransformComponent>();
		MovementComponent* movement = e->getComponent<MovementComponent>();

		// Update velocity
		movement->velocity += (movement->constantAcceleration + movement->accelerationToAdd) * dt;

		// Reset additional acceleration
		movement->accelerationToAdd = glm::vec3(0.0f);

		if (e->getName() == "PlayerCandle") {
			if (movement->velocity.y < 0) {
				movement->velocity.y = 0;
			}
		}
		
		// Rotation
		if (movement->rotation != glm::vec3(0.0f)) {
			transform->rotate(movement->rotation * dt);
		}

		// Set initial value which might be changed in CollisionSystem
		movement->updateableDt = dt;
	}
}


template class MovementSystem<RenderInActiveGameComponent>;
template class MovementSystem<RenderInReplayComponent>;
