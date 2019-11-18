#include "pch.h"
#include "MovementSystem.h"
#include "..//..//components/TransformComponent.h"
#include "..//..//components/ReplayTransformComponent.h"
#include "..//..//components/MovementComponent.h"
#include "..//..//Entity.h"

template <typename T>
MovementSystem<T>::MovementSystem() {
	registerComponent<T>(true, true, true);
	registerComponent<MovementComponent>(true, true, true);
}

template <typename T>
void MovementSystem<T>::update(float dt) {
	for (auto& e : entities) {

		T* transform = e->getComponent<T>();
		MovementComponent* movement = e->getComponent<MovementComponent>();

		// Update velocity
		movement->velocity += (movement->constantAcceleration + movement->accelerationToAdd) * dt;

		// Reset additional acceleration
		movement->accelerationToAdd = glm::vec3(0.0f);
		
		// Rotation
		if (movement->rotation != glm::vec3(0.0f)) {
			transform->rotate(movement->rotation * dt);
		}

		// Set initial value which might be changed in CollisionSystem
		movement->updateableDt = dt;
	}
}

template class MovementSystem<TransformComponent>;
template class MovementSystem<ReplayTransformComponent>;