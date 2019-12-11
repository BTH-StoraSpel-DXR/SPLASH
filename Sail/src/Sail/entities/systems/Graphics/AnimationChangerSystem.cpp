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
				if (velMag2 < 0.1f) {
					animationC->setAnimation(IDLE_DROP);
					if (animIndexBefore != animationC->animationIndex) {
						animationC->animationTime = 0.f;
					}
				}
				continue;
				// Keep for future fixes
			} else if (throwC->isThrowing /*&& !animationC->throwAnimationDone /* || 
				(animationC->nextAnimation == nullptr && 
					   (animationC->animationIndex == IDLE_THROW || animationC->animationIndex == RUNNING_THROW) && 
						(animationC->animationTime + dt * animationC->animationSpeed < animationC->currentAnimation->getMaxAnimationTime()))*/) {
				if (velMag2 < 0.1f) {
					animationC->setAnimation(IDLE_THROW, true);
				} else {
					animationC->setAnimation(RUNNING_THROW, true);
				}
				if (!(animIndexBefore == IDLE_THROW || animIndexBefore == RUNNING_THROW)) {
					animationC->animationTime = 0.f;
				}

				animationC->throwAnimationDone = false;
				continue;
			}

			animationC->throwAnimationDone = false;

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
			else if (moveC->relVel.y > 0.1f && fabsf(moveC->relVel.x) < 0.1f && fabsf(moveC->relVel.z) < 0.1f && 
					 animationC->animationIndex != IDLE_THROW &&
					 animationC->animationIndex != RUNNING_THROW) {
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

				
				if (moveC->relVel.y > 0.1f && (fabsf(moveC->relVel.x) > 0.1f || fabsf(moveC->relVel.z) > 0.1f) &&
					animationC->animationIndex != IDLE_THROW &&
					animationC->animationIndex != RUNNING_THROW) {
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
