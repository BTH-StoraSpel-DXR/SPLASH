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
	const bool isHost = NWrapperSingleton::getInstance().isHost();
	

	//Only let host control sanity for all players.
	if (!isHost) {
		return;
	}

	for (auto& e : entities) {
		SanityComponent* sanityComp = e->getComponent<SanityComponent>();
		TransformComponent* playerTransformComp = e->getComponent<TransformComponent>();
		
		CandleComponent* candleComp = nullptr;
		TransformComponent* candleTransformComp = nullptr;
		Entity* candleEntity = nullptr;

		for (auto& child : e->getChildEntities()) {
			if (!child->isAboutToBeDestroyed() && child->hasComponent<CandleComponent>()) {
				candleComp = child->getComponent<CandleComponent>();
				candleTransformComp = child->getComponent<TransformComponent>();
				candleEntity = child;
				break;
			}
		}

		if (candleEntity) {
			float dist;
			if (candleComp->isCarried && candleComp->isLit) {
				dist = -12;
			}
			else {
				dist = glm::min(glm::distance(playerTransformComp->getTranslation(), candleTransformComp->getTranslation()), 25.f);
			}

			//sanityComp->sanity -= (dist - 1) * dt * 0.5f;
			//sanityComp->sanity = std::clamp(sanityComp->sanity, m_minSanity, m_maxSanity);
			if (sanityComp->sanity <= 0) {
				candleComp->kill(CandleComponent::DamageSource::INSANE, Netcode::INSANITY_COMP_ID);
			}
		}
	}
}

void SanitySystem::updateSanityNetworked(Netcode::ComponentID id, float sanity) {
	bool found = false;

	for (auto& e : entities) {
		NetworkReceiverComponent* networkReceiverComp = nullptr;
		SanityComponent* sanityComp = nullptr;

		if (e->hasComponent<NetworkReceiverComponent>()) {
			networkReceiverComp = e->getComponent<NetworkReceiverComponent>();
			if (networkReceiverComp->m_id == id) {
				found = true;

				if (sanityComp = e->getComponent<SanityComponent>()) {
					sanityComp->sanity = sanity;
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

#ifdef DEVELOPMENT
unsigned int SanitySystem::getByteSize() const {
	return BaseComponentSystem::getByteSize() + sizeof(*this);
}
#endif

bool SanitySystem::onEvent(const Event& event) {
    switch(event.type) {                                                                             
	case Event::Type::SANITY_SYSTEM_UPDATE_SANITY:
		const UpdateSanityEvent& e = static_cast<const UpdateSanityEvent&>(event);
		updateSanityNetworked(e.id, e.sanityVal);
		break;
	 }

	return true;
} 