#include "pch.h"
#include "PlayerSystem.h"
#include "Sail/events/EventDispatcher.h"
#include "Sail/events/types/PlayerDiedEvent.h"
#include "Sail/../Network/NWrapperSingleton.h"
#include "Sail/entities/components/TransformComponent.h"
#include "Sail/entities/components/NoEntityComponent.h"
#include "Sail/entities/Entity.h"

#include <array>

PlayerSystem::PlayerSystem() {
	registerComponent<NoEntityComponent>(true, true, true);
	EventDispatcher::Instance().subscribe(Event::Type::PLAYER_DEATH, this);
}

PlayerSystem::~PlayerSystem() {
	EventDispatcher::Instance().unsubscribe(Event::Type::PLAYER_DEATH, this);
}

bool PlayerSystem::onEvent(const Event& event) {

	auto onPlayerDied = [](const PlayerDiedEvent& e) {
		constexpr float LIFETIME_AFTER_DEATH = 10.f; // Dead entities are removed after 10 seconds

		const auto& myPlayerID = NWrapperSingleton::getInstance().getMyPlayerID();

		// Each bit corresponds to a ComponentType
		// if it's a component we want to keep the bit is set to 1
		const ComponentTypeBitID componentsToKeep = (
			ModelComponent::getBID()
			| CullingComponent::getBID() // needed ??
			| BoundingBoxComponent::getBID()
			| ReplayComponent::getBID()
			| ReplayTransformComponent::getBID()
			);


		// Remove all components from the candle and the gun that aren't needed for the replay
		for (Entity* c : e.killed->getChildEntities()) {
			// Remove the parent from the transform components
			if (c->hasComponent<TransformComponent>()) {
				c->getComponent<TransformComponent>()->removeParent();
			}

			for (ComponentTypeID c_ID = 0; c_ID < BaseComponent::nrOfComponentTypes(); c_ID++) {
				// If the current index is a component that we want to keep leave it be and go to the next one
				if ((GetBIDofID(c_ID) & componentsToKeep).any()) { continue; }

				c->removeComponent(c_ID);
			}
			c->addComponent<LifeTimeComponent>(LIFETIME_AFTER_DEATH);
		}


		// Check if the player was the one who died
		if (Netcode::getComponentOwner(e.netIDofKilled) == myPlayerID) {
			// If my player died, I become a spectator

			// Make this into a real spectator
			e.killed->addComponent<SpectatorComponent>();
			e.killed->getComponent<MovementComponent>()->constantAcceleration = glm::vec3(0.f);
			e.killed->getComponent<MovementComponent>()->velocity = glm::vec3(0.f);
			e.killed->getComponent<NetworkSenderComponent>()->removeAllMessageTypes();
			e.killed->removeComponent<GunComponent>();
			e.killed->removeComponent<AnimationComponent>();
			e.killed->removeComponent<ModelComponent>();
			e.killed->removeComponent<LocalOwnerComponent>();

			// Move entity above the level and make it look down
			auto transform = e.killed->getComponent<TransformComponent>();
			auto pos = glm::vec3(transform->getCurrentTransformState().m_translation);
			pos.y = 20.f;
			transform->setStartTranslation(pos);
			auto& mapSettings = Application::getInstance()->getSettings().gameSettingsDynamic["map"];
			const auto& middleOfLevel = glm::vec3(mapSettings["tileSize"].value * mapSettings["sizeX"].value / 2.f, 0.f, mapSettings["tileSize"].value * mapSettings["sizeY"].value / 2.f);
			const auto& dir = glm::normalize(middleOfLevel - pos);
			const auto& rots = Utils::getRotations(dir);
			transform->setRotations(glm::vec3(0.f, -rots.y, rots.x));
		} else {
			// if it wasn't our player then remove all components that aren't needed for replay and 
			// add lifetime component so that it eventually gets destroyed
			for (ComponentTypeID c_ID = 0; c_ID < BaseComponent::nrOfComponentTypes(); c_ID++) {
				// If the current index is not one of the components that we want to keep then remove it
				if ((GetBIDofID(c_ID) & componentsToKeep).none()) {
					e.killed->removeComponent(c_ID);
				}
			}
			e.killed->addComponent<LifeTimeComponent>(LIFETIME_AFTER_DEATH);
		}
	};


	switch (event.type) {
	case Event::Type::PLAYER_DEATH: onPlayerDied((const PlayerDiedEvent&)event); break;
	default: break;
	}

	return true;
}
