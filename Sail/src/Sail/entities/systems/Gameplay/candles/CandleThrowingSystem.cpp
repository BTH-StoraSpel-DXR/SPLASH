#include "pch.h"
#include "CandleThrowingSystem.h"

#include "Sail/entities/components/Components.h"
#include "Network/NWrapperSingleton.h"
#include "Sail/utils/GameDataTracker.h"
#include "Sail/netcode/NetworkedStructs.h"

#include "Sail/entities/ECS.h"
#include "Sail/entities/systems/physics/UpdateBoundingBoxSystem.h"

CandleThrowingSystem::CandleThrowingSystem() {
	registerComponent<ThrowingComponent>(true, true, true);
	registerComponent<TransformComponent>(true, true, true);
	registerComponent<MovementComponent>(true, true, true);
	registerComponent<NetworkSenderComponent>(false, true, false);

	registerComponent<AnimationComponent>(false, true, true);
	registerComponent<CollisionComponent>(false, true, true);
	registerComponent<CandleComponent>(false, true, true);
	registerComponent<RagdollComponent>(false, true, false);
}

CandleThrowingSystem::~CandleThrowingSystem() {}

void CandleThrowingSystem::setOctree(Octree* octree) {
	m_octree = octree;
}

void CandleThrowingSystem::update(float dt) {
	for (auto& e : entities) {
		if (e->hasComponent<SpectatorComponent>()) {
			continue;
		}
		auto throwC = e->getComponent<ThrowingComponent>();

		Entity* torchE = nullptr;
		for (auto& torch : e->getChildEntities()) {
			if (torch->hasComponent<CandleComponent>() && !torch->isAboutToBeDestroyed()) {
				torchE = torch;
			}
		}

		if (torchE != nullptr) {
			auto candleC = torchE->getComponent<CandleComponent>();
			if (!candleC) {
				SAIL_LOG_WARNING("CandleThrowingSystem::update: Candle component was a nullptr.");
				return;
			}
			if (candleC->isLit) {
				if (throwC->isDropping) {
					throwC->dropTimer += dt;
					if (throwC->dropTimer > DROP_ANIMATION_LENGTH) {
						throwC->isDropping = false;
						throwC->dropTimer = 0.f;
					}
				} else if (throwC->wasChargingLastFrame || throwC->isThrowing) {

					if (throwC->isCharging) {
						// Charging
						throwC->chargeTime += dt;

					} else {
						if (throwC->throwingTimer == 0.f && throwC->chargeTime >= throwC->chargeToThrowThreshold) {
							// Send start throw event
							EventDispatcher::Instance().emit(StartThrowingEvent(e->getComponent<NetworkReceiverComponent>()->m_id));
							NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
								Netcode::MessageType::START_THROWING,
								SAIL_NEW Netcode::MessageStartThrowing{
									e->getComponent<NetworkReceiverComponent>()->m_id
								}
							);
						}

						throwC->throwingTimer += dt;
						// Just drop it
						if (throwC->chargeTime < throwC->chargeToThrowThreshold) {
							throwC->isThrowing = false;
							throwC->isDropping = true;
							throwC->doThrow = true;
							// Begin drop animation
						} else if (throwC->throwingTimer >= CHARGE_AND_THROW_ANIM_LENGTH) {
							// Do the throw
							throwC->isThrowing = false;
							throwC->doThrow = true;
						} else {
							throwC->isThrowing = true;
						}
					}

				}

				if (throwC->doThrow) {
					candleC->candleToggleTimer = 1.8f;

					// Time to throw
					auto transC = torchE->getComponent<TransformComponent>();
					auto moveC = torchE->getComponent<MovementComponent>();

					// Remove the candle from players hand
					glm::vec3 throwPos = transC->getMatrixWithUpdate()[3];
					auto parTrans = e->getComponent<TransformComponent>();
					glm::vec3 parTranslation = parTrans->getMatrixWithUpdate()[3];
					transC->removeParent();
					transC->setRotations(glm::vec3(0.f,0.f,0.f));
					e->getComponent<AnimationComponent>()->rightHandEntity = nullptr;

					// Set velocity and things
					throwC->direction = Application::getInstance()->getCurrentCamera()->getDirection();

					moveC->velocity = throwC->direction * throwC->throwingTimer * throwC->throwChargeMultiplier + e->getComponent<MovementComponent>()->velocity;
					moveC->constantAcceleration = glm::vec3(0.f, 0.0f, 0.f);

					// Making sure the torch isn't dropped inside an object
					auto rayFrom = parTranslation;
					rayFrom.y = throwPos.y;
					Octree::RayIntersectionInfo rayInfo;
					auto rayDir = throwPos - rayFrom;
					auto rayDirNorm = glm::normalize(rayDir);
					m_octree->getRayIntersection(rayFrom, rayDirNorm, &rayInfo, e, 0.1f);
					if (!throwC->isDropping) {
						// Keep this until throw is "flawless"
						/*throwPos += throwC->direction * 0.8f;
						throwPos += moveC->velocity * dt;*/
					}
					if (rayInfo.closestHitIndex != -1 && rayInfo.closestHit < glm::length(rayDir)) {
						// TODO: Fix issue with this happening when looking at floor
						throwPos = rayFrom + (rayInfo.closestHit - 0.1f) * rayDirNorm;
					}


					// Set initial throw position
					transC->setTranslation(throwPos);

					if (e->hasComponent<RagdollComponent>()) {
						transC->setCenter(e->getComponent<RagdollComponent>()->localCenterOfMass);
					}

					// Throw the torch
					moveC->velocity = throwC->direction * throwC->throwingTimer * throwC->throwChargeMultiplier + e->getComponent<MovementComponent>()->velocity;
					moveC->constantAcceleration = glm::vec3(0.f, 0.f, 0.f);
					//throwC->direction.y = 0.f;
					auto rotationAxis = glm::normalize(glm::cross(throwC->direction, glm::vec3(0.f, 1.f, 0.f)));
					moveC->rotation = rotationAxis * -6.14f * 2.0f;
					auto* collComp = torchE->addComponent<CollisionComponent>(true);
					collComp->bounciness = 0.2f;
					collComp->drag = 50.0f;
					ECS::Instance()->getSystem<UpdateBoundingBoxSystem>()->update(0.0f);

					// Send stop throw event
					if (throwC->throwingTimer >= CHARGE_AND_THROW_ANIM_LENGTH) {
						EventDispatcher::Instance().emit(StopThrowingEvent(e->getComponent<NetworkReceiverComponent>()->m_id));
						NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
							Netcode::MessageType::STOP_THROWING,
							SAIL_NEW Netcode::MessageStopThrowing{
								e->getComponent<NetworkReceiverComponent>()->m_id
							}
						);
					}

					// Reset values
					throwC->chargeTime = 0.f;
					throwC->throwingTimer = 0.f;
					throwC->doThrow = false;
					//throwC->isDropping = false;
					candleC->isCarried = false;


					continue;
				}
			} else {
				// Candle isn't lit, ABORT MISSION!!!!!
				throwC->chargeTime = 0.f;
				throwC->throwingTimer = 0.f;
				throwC->dropTimer = 0.f;

				throwC->isCharging = false;
				throwC->wasChargingLastFrame = false;
				throwC->isThrowing = false;
				throwC->doThrow = false;
				throwC->isDropping = false;
			}
		}

		throwC->wasChargingLastFrame = throwC->isCharging;
	}
}

#ifdef DEVELOPMENT
unsigned int CandleThrowingSystem::getByteSize() const {
	return BaseComponentSystem::getByteSize() + sizeof(*this);
}
#endif

void CandleThrowingSystem::throwCandle(Entity* e) {}
