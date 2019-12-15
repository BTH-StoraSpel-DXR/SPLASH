#include "pch.h"

#include "HostSendToSpectatorSystem.h"

#include "Sail/entities/components/NetworkSenderComponent.h"
#include "Sail/entities/components/OnlineOwnerComponent.h"
#include "Sail/entities/components/LocalOwnerComponent.h"
#include "Sail/entities/Entity.h"

#include "Network/NWrapperSingleton.h"

#include "cereal/archives/portable_binary.hpp"
#include "gzip/compress.hpp"


HostSendToSpectatorSystem::HostSendToSpectatorSystem() {
	registerComponent<NetworkSenderComponent>(true, true, true);
	registerComponent<TransformComponent>(true, true, false);
	//registerComponent<AudioComponent>(false, true, true);
	registerComponent<CandleComponent>(true, true, true);
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
		//if (e->hasComponent<AudioComponent>()) {
		//	for (int i = 0; i < Audio::SoundType::COUNT; i++) {
		//		e->getComponent<AudioComponent>()->m_sounds[i].isPlaying = false;
		//	}
		//}


		if (e->getComponent<NetworkSenderComponent>()->m_entityType == Netcode::EntityType::CANDLE_ENTITY) {
			if (e->hasComponent<CandleComponent>() && !e->getComponent<CandleComponent>()->isCarried) {
				nrOfEvents++;
			}
		}
	}

	ar(nrOfEvents);

	// Loop through the entities and send messages the necessary information needed set the correct candle state for players
	for (auto e : entities) {
		NetworkSenderComponent* nsc = e->getComponent<NetworkSenderComponent>();

		switch (nsc->m_entityType) {
		case Netcode::EntityType::CANDLE_ENTITY:
		{
			// Find the component ID of the player's candle
			Netcode::ComponentID candleID = e->getComponent<NetworkSenderComponent>()->m_id;
			//for (auto c : e->getChildEntities()) {
			//	if (c->hasComponent<CandleComponent>()) {
			//		candleID = c->getComponent<NetworkSenderComponent>()->m_id;
			//	}
			//}

			//for (auto c : e->getChildEntities()) {
			if (e->hasComponent<CandleComponent>() && !e->getComponent<CandleComponent>()->isCarried) {
				ar(Netcode::MessageType::CANDLE_HELD_STATE);
				ar(nsc->m_id);
				ar(false);
			}
			//}

		}
		break;
		default:
			break;
		}

	}

	// TODO: send candle held events too



	// -+-+-+-+-+-+-+-+ send the serialized archive over the network -+-+-+-+-+-+-+-+ 
	std::string uncompressed = dataString.str();
	const char* uncompressedPtr = uncompressed.data();
	std::string compressed = gzip::compress(uncompressedPtr, uncompressed.size());

	NWrapperSingleton::getInstance().getNetworkWrapper()->sendSerializedDataToClient(compressed, PlayerId);
}

#ifdef DEVELOPMENT
unsigned int HostSendToSpectatorSystem::getByteSize() const {
	return BaseComponentSystem::getByteSize() + sizeof(*this);
}
#endif