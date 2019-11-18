#include "pch.h"
#include "SanitySystem.h"
#include "Sail/entities/components/SanityComponent.h"
#include "Sail/entities/components/CandleComponent.h"
#include "Sail/entities/components/TransformComponent.h"
#include "Sail/entities/components/NetworkReceiverComponent.h"

#include "Sail/entities/Entity.h"
#include "Network/NWrapperSingleton.h"
#include "Sail/events/Event.h"
#include "Sail/events/Events.h"

SanitySystem::SanitySystem() {
	registerComponent<SanityComponent>(true, true, true);
	registerComponent<TransformComponent>(true, true, false);
	registerComponent<CandleComponent>(false, true, true);

	EventDispatcher::Instance().subscribe(Event::Type::SANITY_SYSTEM_UPDATE_SANITY, this);
}

SanitySystem::~SanitySystem() {
	EventDispatcher::Instance().unsubscribe(Event::Type::SANITY_SYSTEM_UPDATE_SANITY, this);
}

void SanitySystem::update(float dt) {
	bool isHost = NWrapperSingleton::getInstance().isHost();
	
	//Only let host controll sanity for all players.
	if (!isHost) {
		return;
	}

	for (auto& e : entities) {
		SanityComponent* ic = e->getComponent<SanityComponent>();
		TransformComponent* tc = e->getComponent<TransformComponent>();
		
		CandleComponent* cc = nullptr;
		TransformComponent* c_tc = nullptr;
		Entity* candle_entity = nullptr;

		for (auto& child : e->getChildEntities()) {
			if ((cc = child->getComponent<CandleComponent>()) && (c_tc = child->getComponent<TransformComponent>())) {
				candle_entity = child;
				break;
			}
		}

		if (candle_entity) {
			float dist;
			if (cc->isCarried && cc->isLit) {
				dist = -12;
			} else {
				dist = glm::distance(tc->getTranslation(), c_tc->getTranslation());
			}

			ic->sanity -= (dist - 1) * dt * 0.5;
			ic->sanity = std::clamp(ic->sanity, m_minSanity, m_maxSanity);
			if (ic->sanity <= 0) {
				cc->kill(CandleComponent::DamageSource::INSANE, Netcode::MESSAGE_INSANITY_ID);
			}
		}
	}
}

void SanitySystem::updateSanityNetworked(Netcode::ComponentID id, float sanity) {
	bool found = false;

	for (auto& e : entities) {
		NetworkReceiverComponent* nc = nullptr;
		SanityComponent* ic = nullptr;

		if (nc = e->getComponent<NetworkReceiverComponent>()) {
			if (nc->m_id == id) {
				found = true;

				if (ic = e->getComponent<SanityComponent>()) {
					ic->sanity = sanity;
				} else {
					SAIL_LOG_WARNING("Tried to update sanity on an entity without a sanityComponent!\n");
				}
				break;
			}
		}
	}

#ifdef DEVELOPMENT
	if (!found) {
		SAIL_LOG_WARNING("Tried to update sanity on an entity that do not exist!\n");
	}
#endif

}

bool SanitySystem::onEvent(const Event& event) {
    switch(event.type) {                                                                             
	case Event::Type::SANITY_SYSTEM_UPDATE_SANITY:
		const UpdateSanityEvent& e = static_cast<const UpdateSanityEvent&>(event);
		updateSanityNetworked(e.id, e.sanityVal);
		break;
	 }

	return true;
} 