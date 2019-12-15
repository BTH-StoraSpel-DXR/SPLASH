#include "pch.h"
#include "CandlePlacementSystem.h"

#include "Sail/entities/components/Components.h"
#include "Network/NWrapperSingleton.h"
#include "Sail/utils/GameDataTracker.h"

#include "Sail/entities/ECS.h"
#include "Sail/entities/systems/physics/UpdateBoundingBoxSystem.h"

#include "Sail/events/EventDispatcher.h"
#include "Sail/events/types/HoldingCandleToggleEvent.h"
#include "Sail/events/types/TorchNotHeldEvent.h"

CandlePlacementSystem::CandlePlacementSystem() {
	registerComponent<CandleComponent>(true, true, true);
	registerComponent<TransformComponent>(true, true, true);
	registerComponent<NetworkSenderComponent>(false, true, false);

	registerComponent<AnimationComponent>(false, true, true);
	registerComponent<CollisionComponent>(false, true, true);

	EventDispatcher::Instance().subscribe(Event::Type::HOLDING_CANDLE_TOGGLE, this);
}

CandlePlacementSystem::~CandlePlacementSystem() {
	EventDispatcher::Instance().unsubscribe(Event::Type::HOLDING_CANDLE_TOGGLE, this);
}

void CandlePlacementSystem::update(float dt) {
	for (auto e : entities) {
		if (e->isAboutToBeDestroyed()) {
			continue;
		}

		auto candle = e->getComponent<CandleComponent>();
		if (candle->isCarried != candle->wasCarriedLastUpdate) {
			toggleCandlePlacement(e);
		} else if (!candle->isLit && !candle->isCarried) {
			candle->isCarried = true;
			toggleCandlePlacement(e);
		}

		candle->wasCarriedLastUpdate = candle->isCarried;
		constexpr float candleHeight = 0.37f;
		glm::vec3 flamePos = e->getComponent<TransformComponent>()->getMatrixWithUpdate() * glm::vec4(0, candleHeight, 0, 1);

		e->getComponent<LightComponent>()->currentPos = flamePos;

		if (!candle->isCarried && e->getParent() && e->getParent()->hasComponent<NetworkReceiverComponent>()) {
			EventDispatcher::Instance().emit(TorchNotHeldEvent(e->getParent()->getComponent<NetworkReceiverComponent>()->m_id));
		}
	}
}

void CandlePlacementSystem::toggleCandlePlacement(Entity* e) {
	auto candleComp = e->getComponent<CandleComponent>();
	auto candleTransComp = e->getComponent<TransformComponent>();
	auto parentTransComp = e->getParent()->getComponent<TransformComponent>();

	if (candleComp->isCarried) {
		// Pick up the candle
		if (glm::length(parentTransComp->getTranslation() - candleTransComp->getTranslation()) < 2.0f || !candleComp->isLit) {
			candleTransComp->setParent(parentTransComp);
			e->getParent()->getComponent<AnimationComponent>()->rightHandEntity = e;
			e->removeComponent<CollisionComponent>();
			if (e->hasComponent<MovementComponent>()) {
				auto moveC = e->getComponent<MovementComponent>();
				moveC->constantAcceleration = glm::vec3{ 0, 0, 0 };
				moveC->velocity = glm::vec3(0.f);
				moveC->oldVelocity = glm::vec3(0.f);
			}
			candleTransComp->setCenter(glm::vec3(0.0f));
			

		} else {
			candleComp->isCarried = false;
		}
	}

	// Only send the message if we actually did something to the candle
	if (candleComp->isCarried != candleComp->wasCarriedLastUpdate) {
		// Inform other players that we've put down our candle
		if (e->getParent()->getComponent<LocalOwnerComponent>()) {
			NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
				Netcode::MessageType::CANDLE_HELD_STATE,
				SAIL_NEW Netcode::MessageCandleHeldState{
					e->getParent()->getComponent<NetworkSenderComponent>()->m_id,
					candleComp->isCarried,
					e->getComponent<TransformComponent>()->getTranslation()
				},
				false // We've already put down our own candle so no need to do it again
			);

			auto senderC = e->getComponent<NetworkSenderComponent>();

			if (candleComp->isCarried) {
				// If we're holding the candle it will be attached to our animation so don't send its
				// position and rotation.
				senderC->removeMessageType(Netcode::MessageType::CHANGE_LOCAL_POSITION);
				senderC->removeMessageType(Netcode::MessageType::CHANGE_LOCAL_ROTATION);
				NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
					Netcode::MessageType::SET_CENTER,
					SAIL_NEW Netcode::MessageSetCenter{
						e->getComponent<NetworkReceiverComponent>()->m_id,
						glm::vec3(0.0f)
					}, false
				);
			} else {
				// If we're no longer holding the candle then start sending its position and rotation
				// so that other people will be able to see it when we throw it.
				senderC->addMessageType(Netcode::MessageType::CHANGE_LOCAL_POSITION);
				senderC->addMessageType(Netcode::MessageType::CHANGE_LOCAL_ROTATION);
			}
		}
	}
}

bool CandlePlacementSystem::onEvent(const Event& event) {
	auto onHoldingCandleToggle = [&](const HoldingCandleToggleEvent& e) {
		Entity* player = nullptr;
		Entity* candle = nullptr;

		// Find the candle whose parent has the correct ID
		for (auto candleEntity : entities) {
			if (auto parentEntity = candleEntity->getParent(); parentEntity) {
				if (parentEntity->getComponent<NetworkReceiverComponent>()->m_id == e.netCompID) {
					player = parentEntity;
					candle = candleEntity;
					break;
				}
			}
		}

		// candle exists => player exists (only need to check candle)
		if (!candle) {
			SAIL_LOG_WARNING("Holding candle toggled but no matching entity found");
			return;
		}

		auto candleComp = candle->getComponent<CandleComponent>();
		auto candleTransComp = candle->getComponent<TransformComponent>();

		candleComp->isCarried = e.isHeld;
		candleComp->wasCarriedLastUpdate = e.isHeld;
		if (e.isHeld) {
			candleTransComp->setTranslation(glm::vec3(10.f, 2.0f, 0.f));
			candleTransComp->setParent(player->getComponent<TransformComponent>());

			player->getComponent<AnimationComponent>()->rightHandEntity = candle;
			if (candle->hasComponent<MovementComponent>()) {
				if (candle->hasComponent<MovementComponent>()) {
					auto moveC = candle->getComponent<MovementComponent>();
					moveC->constantAcceleration = glm::vec3{ 0.0f, 0.0f, 0.0f };
					moveC->velocity = glm::vec3(0.f);
					moveC->oldVelocity = glm::vec3(0.f);
				}
			}
		} else {
			candleTransComp->removeParent();
			player->getComponent<AnimationComponent>()->rightHandEntity = nullptr;

			// Might be needed
			ECS::Instance()->getSystem<UpdateBoundingBoxSystem>()->update(0.0f);
		}
	};

	switch (event.type) {
	case Event::Type::HOLDING_CANDLE_TOGGLE: onHoldingCandleToggle((const HoldingCandleToggleEvent&)event); break;
	default: break;
	}

	return true;
}
