#include "pch.h"

#include "HostSendToSpectatorSystem.h"

#include "Sail/entities/components/NetworkSenderComponent.h"
#include "Sail/entities/components/OnlineOwnerComponent.h"
#include "Sail/entities/components/LocalOwnerComponent.h"
#include "Sail/entities/Entity.h"

#include "Network/NWrapperSingleton.h"
#include "Sail/../../libraries/cereal/archives/portable_binary.hpp"

#include "../src/Network/NWrapperSingleton.h"

HostSendToSpectatorSystem::HostSendToSpectatorSystem() {
	registerComponent<NetworkSenderComponent>(true, true, true);
	registerComponent<TransformComponent>(true, true, false);
}

HostSendToSpectatorSystem::~HostSendToSpectatorSystem() {
}

void HostSendToSpectatorSystem::init(Netcode::PlayerID playerID) {
	m_playerID = playerID;
}


// The messages this function creates must match the format of messages created by and received by NetworkSenderSystem and NetworkReceiverSystem
void HostSendToSpectatorSystem::sendEntityCreationPackage(Netcode::PlayerID PlayerId) const {
	std::ostringstream dataString(std::ios::binary);
	Netcode::OutArchive ar(dataString);

	// -+-+-+-+-+-+-+-+ Per-frame sends to per-frame receives via components -+-+-+-+-+-+-+-+ 
	// Send our playerID so that we can ignore this packet when it gets back to us from the host
	ar(m_playerID);

	// Don't send any data from NetworkSenderComponents.
	ar(size_t{ 0 });



	// TODO: if a player's candle isn't held also send an event for that
	size_t nrOfEvents = 0;
	bool candleHeld = true;

	// Find how many players are alive so that the receiver knows how many messages to receive
	for (auto e : entities) {
		if (e->getComponent<NetworkSenderComponent>()->m_entityType == Netcode::EntityType::PLAYER_ENTITY) {
			nrOfEvents++;

			for (auto c : e->getChildEntities()) {
				if (c->hasComponent<CandleComponent>() && !c->getComponent<CandleComponent>()->isCarried) {
					nrOfEvents++;
				}
			}
		}
	}

	ar(nrOfEvents);

	// Loop through the entities and send messages the necessary information needed to create the entities for the spectator
	for (auto e : entities) {
		NetworkSenderComponent* nsc = e->getComponent<NetworkSenderComponent>();

		// When creating a player the code here must match the code in
		// NetworkSenderSystem::writeEventToArchive()
		// ... case Netcode::MessageType::CREATE_NETWORKED_PLAYER:
		switch (nsc->m_entityType) {
		case Netcode::EntityType::PLAYER_ENTITY:
		{
			Netcode::ComponentID candleID, gunID;
			// Find the component ID of the player's candle
			for (auto c : e->getChildEntities()) {
				if (c->hasComponent<CandleComponent>()) {
					candleID = c->getComponent<NetworkSenderComponent>()->m_id;
				}
				else {
					if (e->hasComponent<GunComponent>()) {
						gunID = c->getComponent<NetworkSenderComponent>()->m_id;
					}
				}


			}
			//if (e->hasComponent<GunComponent>()) {
			//	gunID = e->getComponent<NetworkSenderComponent>()->m_id;
			//}
			glm::vec3 position = e->getComponent<TransformComponent>()->getCurrentTransformState().m_translation;

			ar(Netcode::MessageType::CREATE_NETWORKED_PLAYER);
			ar(nsc->m_id); // Send the player's componentID
			ar(candleID);  // Send the player's candle's componentID
			ar(gunID);     // Send the player's gun's componentID
			ArchiveHelpers::saveVec3(ar, position); // Send the player's current position


			for (auto c : e->getChildEntities()) {
				if (c->hasComponent<CandleComponent>() && !c->getComponent<CandleComponent>()->isCarried) {
					ar(Netcode::MessageType::CANDLE_HELD_STATE);
					ar(nsc->m_id);
					ar(false);
				}
			}


		}
		break;
		default:
			break;
		}

	}

	// TODO: send candle held events too



	// -+-+-+-+-+-+-+-+ send the serialized archive over the network -+-+-+-+-+-+-+-+ 
	std::string binaryData = dataString.str();
	NWrapperSingleton::getInstance().getNetworkWrapper()->sendSerializedDataToClient(binaryData, PlayerId);
}
