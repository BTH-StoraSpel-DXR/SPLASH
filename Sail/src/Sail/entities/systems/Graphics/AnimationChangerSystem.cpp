#include "pch.h"
#include "AnimationChangerSystem.h"
#include "../../ECS.h"
#include "../../components/TransformComponent.h"
#include "../../components/AnimationComponent.h"
#include "../../components/MovementComponent.h"
#include "../../components/ThrowingComponent.h"
#include "../../components/SprintingComponent.h"

AnimationChangerSystem::AnimationChangerSystem() {
	registerComponent<AnimationComponent>(true, true, true);
	registerComponent<MovementComponent>(true, true, false);
	registerComponent<ThrowingComponent>(true, true, false);
	registerComponent<SprintingComponent>(true, true, false);
}



AnimationChangerSystem::~AnimationChangerSystem() {

}

void AnimationChangerSystem::update(float dt) {

	for (auto& e : entities) {
		AnimationComponent* animationC = e->getComponent<AnimationComponent>();
		MovementComponent* moveC = e->getComponent<MovementComponent>();
		auto throwC = e->getComponent<ThrowingComponent>();
		if (animationC && moveC && throwC) {
			animationC->updateDT = true;

			/*
				THROWING ANIMATIONS
			*/
			auto animIndexBefore = animationC->animationIndex;
			float velMag2 = glm::length2(moveC->velocity);
			if (throwC->isDropping) {
				if (velMag2 < 0.01f) {
					animationC->setAnimation(IDLE_DROP);
				} else {
					// Replace the animation indices once they are implemented
					animationC->setAnimation(IDLE_DROP);
					//animationC->setAnimation(RUNNING_DROP);
				}
				if (animIndexBefore != animationC->animationIndex) {
					animationC->animationTime = 0.f;
				}
				continue;
			} else if (throwC->isCharging || throwC->isThrowing) {
				//animationC->updateDT = false;
				if (velMag2 < 0.01f) {
					// Replace the animation indices once they are implemented
					animationC->setAnimation(IDLE_THROW);
				} else {
					// Replace the animation indices once they are implemented
					animationC->setAnimation(IDLE_THROW);
					//animationC->setAnimation(RUNNING_THROW);
				}
				if (animIndexBefore != animationC->animationIndex) {
					animationC->animationTime = 0.f;
				}
				continue;
			// To make sure the animation isn't swapped before the animation is done
			} else if ((animationC->animationIndex == 255/*IDLE_JUMP*/ ||
					   animationC->animationIndex == IDLE_THROW) &&
					   animationC->animationTime < (animationC->currentAnimation->getMaxAnimationTime() - dt)) {
				continue;
			}

			auto sprintC = e->getComponent<SprintingComponent>();
			if (sprintC->doSprint && sprintC->canSprint) {
				animationC->setAnimation(SPRINTING);
				continue;
			}

			
			if (animationC->animationIndex == IDLEJUMP && animationC->animationTime < 0.1f) {
				animationC->setAnimation(IDLE);
			}

			if (velMag2 < 0.01f && animationC->animationIndex != IDLEJUMP) {
				animationC->setAnimation(IDLE);
			} 
			else if (moveC->relVel.y > 0.1f && fabsf(moveC->relVel.x) < 0.1f && fabsf(moveC->relVel.z) < 0.1f) {
				if (animationC->animationIndex != IDLEJUMP) {
					animationC->setAnimation(IDLEJUMP);
					animationC->animationTime = 0.310f;
				}
				
			}
			else {

				if (animationC->animationIndex == FORWARDJUMP) {
					if (animationC->animationTime < 0.1f) {
						animationC->setAnimation(IDLE);
					}
					else {
						continue;
					}
				}

				
				if (moveC->relVel.y > 0.1f && (fabsf(moveC->relVel.x) > 0.1f || fabsf(moveC->relVel.z) > 0.1f)) {
					animationC->setAnimation(FORWARDJUMP);
					animationC->animationTime = 0.5f;
				}
				else if (fabsf(moveC->relVel.x) > fabsf(moveC->relVel.z)) {
					if (moveC->relVel.x > 0.1f) {
						animationC->setAnimation(FORWARD);
					} 
					else if (moveC->relVel.x < -0.1f) {
						animationC->setAnimation(BACKWARD);
					}
				} 
				else {
					if (moveC->relVel.z > 0.1f) {
						animationC->setAnimation(RIGHT);
					} else if (moveC->relVel.z < -0.1f) {
						animationC->setAnimation(LEFT);
					}
				}
			}
		}
	}
}
