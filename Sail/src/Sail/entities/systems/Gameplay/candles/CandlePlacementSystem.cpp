#include "pch.h"
#include "CandlePlacementSystem.h"

#include "Sail/entities/components/Components.h"
#include "Network/NWrapperSingleton.h"
#include "Sail/utils/GameDataTracker.h"

#include "Sail/entities/ECS.h"
#include "Sail/entities/systems/physics/UpdateBoundingBoxSystem.h"
#include "glm/gtx/vector_angle.hpp"

CandlePlacementSystem::CandlePlacementSystem() {
	// TODO: System owner should check if this is correct
	registerComponent<CandleComponent>(true, true, true);
	registerComponent<TransformComponent>(true, true, true);
	registerComponent<NetworkSenderComponent>(false, true, false);
}

CandlePlacementSystem::~CandlePlacementSystem() {}

void CandlePlacementSystem::setOctree(Octree* octree) {
	m_octree = octree;
}

void CandlePlacementSystem::update(float dt) {
	for (auto e : entities) {
		auto candle = e->getComponent<CandleComponent>();
		if (candle->isCarried != candle->wasCarriedLastUpdate) {
			putDownCandle(e);

			// Inform other players that we've put down our candle
			if (e->getParent()->getComponent<LocalOwnerComponent>()) {
				NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
					Netcode::MessageType::CANDLE_HELD_STATE,
					SAIL_NEW Netcode::MessageCandleHeldState{
						e->getParent()->getComponent<NetworkSenderComponent>()->m_id,
						candle->isCarried,
						e->getComponent<TransformComponent>()->getTranslation()
					},
					false // We've already put down our own candle so no need to do it again
				);
			}
		}

		candle->wasCarriedLastUpdate = candle->isCarried;
		static const float candleHeight = 0.37f;
		glm::vec3 flamePos = e->getComponent<TransformComponent>()->getMatrix() * glm::vec4(0, candleHeight, 0, 1);
		e->getComponent<LightComponent>()->getPointLight().setPosition(flamePos);
	}
}

void CandlePlacementSystem::putDownCandle(Entity* e) {
	auto candleComp = e->getComponent<CandleComponent>();
	auto candleTransComp = e->getComponent<TransformComponent>();
	auto parentTransComp = e->getParent()->getComponent<TransformComponent>();

	if (!candleComp->isCarried) {
		if (candleComp->isLit) {
			glm::vec3 parentPos = parentTransComp->getMatrix()[3];
			glm::vec3 dir = candleTransComp->getParent()->getForward();
			dir.y = 0.0f;
			dir = glm::normalize(dir) / 2.0f;
			glm::vec3 candleTryPosition = glm::vec3(parentPos.x - dir.x, parentPos.y, parentPos.z - dir.z);

			bool blocked = false;
			glm::vec3 down(0.f, -1.f, 0.f);
			float heightOffsetFromPlayerFeet = 1.f;

			{
				Octree::RayIntersectionInfo tempInfo;
				// Shoot a ray straight down 1 meter ahead of the player to check for floor
				m_octree->getRayIntersection(glm::vec3(candleTryPosition.x, candleTryPosition.y + heightOffsetFromPlayerFeet, candleTryPosition.z), down, &tempInfo, nullptr, 0.01f);
				if (tempInfo.closestHitIndex != -1) {
					float floorCheckVal = glm::angle(tempInfo.info[tempInfo.closestHitIndex].intersectionAxis, -down);
					// If there's a low angle between the up-vector and the normal of the surface, it can be counted as floor
					bool isFloor = (floorCheckVal < 0.1f) ? true : false;
					if (!isFloor) {
						blocked = true;
					} else {
						// Update the height of the candle position
						candleTryPosition.y = candleTryPosition.y + (heightOffsetFromPlayerFeet - tempInfo.closestHit);
					}
				} else {
					blocked = true;
				}
			}

			{
				Octree::RayIntersectionInfo tempInfo;
				// Check if the position is visible for the player
				auto playerHead = glm::vec3(parentPos.x, parentPos.y + 1.8f, parentPos.z);
				auto playerHeadToCandle = candleTryPosition - playerHead;
				float eps = 0.0001f;
				m_octree->getRayIntersection(playerHead, glm::normalize(playerHeadToCandle), &tempInfo, nullptr);
				float phtcLength = glm::length(playerHeadToCandle);
				if (tempInfo.closestHit - phtcLength + eps < 0.f) {
					// Can't see the position where we try to place the candle
					blocked = true;
				}
			}

			// Place down the candle if it's not blocked
			if (!blocked) {
				candleTransComp->removeParent();
				candleTransComp->setTranslation(candleTryPosition);

				candleTransComp->setRotations(glm::vec3{0.f,0.f,0.f});
				e->getParent()->getComponent<AnimationComponent>()->leftHandEntity = nullptr;

				ECS::Instance()->getSystem<UpdateBoundingBoxSystem>()->update(0.0f);
			} else {
				candleComp->isCarried = true;
			}
		} else {
			candleComp->isCarried = true;
		}
	} else {
		// Pick up the candle
		if (glm::length(parentTransComp->getTranslation() - candleTransComp->getTranslation()) < 2.0f || !candleComp->isLit) {
			candleTransComp->setParent(parentTransComp);
			e->getParent()->getComponent<AnimationComponent>()->leftHandEntity = e;
		} else {
			candleComp->isCarried = false;
		}
	}
}