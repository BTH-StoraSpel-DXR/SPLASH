#include "pch.h"
#include "NetworkSenderSystem.h"

#include "Sail/entities/components/NetworkSenderComponent.h"
#include "Sail/entities/components/OnlineOwnerComponent.h"
#include "Sail/entities/components/LocalOwnerComponent.h"
#include "Sail/entities/Entity.h"

#include "Network/NWrapperSingleton.h"
#include "Sail/../../libraries/cereal/archives/portable_binary.hpp"

#include "../src/Network/NWrapperSingleton.h"

#include <vector>

NetworkSenderSystem::NetworkSenderSystem() : BaseComponentSystem() {
	registerComponent<NetworkSenderComponent>(true, true, true);
	registerComponent<TransformComponent>(false, true, false);
}

NetworkSenderSystem::~NetworkSenderSystem() {
	while (eventQueue.size() > 0) {
		NetworkSenderEvent* pEvent = eventQueue.front();
		eventQueue.pop();
		delete pEvent;
	}
}

void NetworkSenderSystem::initWithPlayerID(unsigned char playerID) {
	m_playerID = playerID;
}

void NetworkSenderSystem::initPlayerEntity(Entity* pPlayerEntity) {
	m_playerEntity = pPlayerEntity;
}

/*
  The construction of messages needs to match how the NetworkReceiverSystem parses them so
  any changes made here needs to be made there as well!

  Logical structure of the package that will be sent by this function:

	__int32         nrOfEntities
	__int32         senderID

		NetworkObjectID entity[0].id
		EntityType		entity[0].type
		__int32			nrOfMessages
			MessageType     entity[0].messageType
			MessageData     entity[0].data

		NetworkObjectID entity[0].id
		__int32			nrOfMessages
			MessageType     entity[0].messageType
			MessageData     entity[0].data

		NetworkObjectID entity[0].id
		__int32			nrOfMessages
			MessageType     entity[0].messageType
			MessageData     entity[0].data
		....

*/
void NetworkSenderSystem::update() {
	using namespace Netcode;

	// Loop through networked entities and serialize their data
	std::ostringstream os(std::ios::binary);
	cereal::PortableBinaryOutputArchive ar(os);

	// TODO: Add game tick here in the future

	// -+-+-+-+-+-+-+-+ Per-frame sends to per-frame receives via components -+-+-+-+-+-+-+-+ 
	// Write nrOfEntities
	ar(static_cast<__int32>(entities.size()));


	// Send our playerID so that we can ignore this packet when it gets back to us from the host
	ar(m_playerID);

	for (auto e : entities) {
		NetworkSenderComponent* nsc = e->getComponent<NetworkSenderComponent>();
		ar(nsc->m_id);										// NetworkObjectID
		ar(nsc->m_entityType);								// Entity type
		ar(static_cast<__int32>(nsc->m_dataTypes.size()));	// NrOfMessages

		// Per type of data
		for (auto& messageType : nsc->m_dataTypes) {
			ar(messageType);								// Current MessageType

			handleEvent(messageType, e, &ar);				// Add to archive depending on the message
		}
	}

	// -+-+-+-+-+-+-+-+ Per-instance events via eventQueue -+-+-+-+-+-+-+-+ 
	unsigned __int32 queueSize = static_cast<__int32>(eventQueue.size());
	ar(queueSize);

	while (eventQueue.empty() == false) {
		NetworkSenderEvent* pE = eventQueue.front();		// Fetch
		handleEvent(pE, &ar);								// Deal with
		eventQueue.pop();									// Pop.
		delete pE;											// Delete
	}

	// -+-+-+-+-+-+-+-+ send the serialized archive over the network -+-+-+-+-+-+-+-+ 
	std::string binaryData = os.str();
	if (NWrapperSingleton::getInstance().isHost()) {
		NWrapperSingleton::getInstance().getNetworkWrapper()->sendSerializedDataAllClients(binaryData);
	} else {
		NWrapperSingleton::getInstance().getNetworkWrapper()->sendSerializedDataToHost(binaryData);
	}



	std::scoped_lock lock(m_forwardBufferLock);
	// The host forwards all the incoming messages they have to all the clients
	while (!m_HOSTONLY_dataToForward.empty()) {
		std::string dataFromClient = m_HOSTONLY_dataToForward.front();


		// This if statement shouldn't be needed since m_dataToForwardToClients will be empty unless you're the host
		if (NWrapperSingleton::getInstance().isHost()) {
			NWrapperSingleton::getInstance().getNetworkWrapper()->sendSerializedDataAllClients(dataFromClient);
		}
		m_HOSTONLY_dataToForward.pop();
	}
}

const void NetworkSenderSystem::queueEvent(NetworkSenderEvent* type) {
	eventQueue.push(type);
}


// ONLY DO FOR THE HOST
// Push incoming data strings to the back of a FIFO list which will be forwarded to all other players
void NetworkSenderSystem::pushDataToBuffer(std::string data) {
	std::scoped_lock lock(m_forwardBufferLock);
	m_HOSTONLY_dataToForward.push(data);
}


// Why is this its own function?
// Can't it just be a message in the normal update()?
void NetworkSenderSystem::stop() {


	using namespace Netcode;

	// Loop through networked entities and serialize their data.
	std::ostringstream os(std::ios::binary);
	cereal::PortableBinaryOutputArchive ar(os);

	// TODO: Add game tick here in the future

	// -+-+-+-+-+-+-+-+ Per-frame sends to per-frame receives via components -+-+-+-+-+-+-+-+ 
	// Write nrOfEntities
	ar(static_cast<__int32>(0));

	// Send our playerID so that we can ignore this packet when it gets back to us from the host
	ar(m_playerID);

	// -+-+-+-+-+-+-+-+ Per-instance events via eventQueue -+-+-+-+-+-+-+-+ 
	//__int32 test = static_cast<__int32>(eventQueue.size());
	bool ended = false;
	while (eventQueue.empty() == false) {
		NetworkSenderEvent* pE = eventQueue.front();		// Fetch
		if ((pE->type == Netcode::MessageType::MATCH_ENDED || pE->type == Netcode::MessageType::SEND_ALL_BACK_TO_LOBBY) && ended == false) {
			ended = true;
			ar(static_cast<__int32>(1));
			handleEvent(pE, &ar);
		}
		eventQueue.pop();									// Pop
		delete pE;											// Delete
	}

	if (!ended) {
		ar(static_cast<__int32>(0));
	} else {
		// send the serialized archive over the network
		std::string binaryData = os.str();
		if (NWrapperSingleton::getInstance().isHost()) {
			NWrapperSingleton::getInstance().getNetworkWrapper()->sendSerializedDataAllClients(binaryData);
		}
		else {
			NWrapperSingleton::getInstance().getNetworkWrapper()->sendSerializedDataToHost(binaryData);
		}


		std::scoped_lock lock(m_forwardBufferLock);
		// The host forwards all the incoming messages they have to all the clients
		while (!m_HOSTONLY_dataToForward.empty()) {
			std::string dataFromClient = m_HOSTONLY_dataToForward.front();


			// This if statement shouldn't be needed since m_dataToForwardToClients will be empty unless you're the host
			if (NWrapperSingleton::getInstance().isHost()) {
				NWrapperSingleton::getInstance().getNetworkWrapper()->sendSerializedDataAllClients(dataFromClient);
			}
			m_HOSTONLY_dataToForward.pop();
		}
	}
}

void NetworkSenderSystem::addEntityToListONLYFORNETWORKRECIEVER(Entity* e) {
	entities.push_back(e);
}

void NetworkSenderSystem::handleEvent(Netcode::MessageType& messageType, Entity* e, cereal::PortableBinaryOutputArchive* ar) {
	// Package it depending on the type
	switch (messageType) {
		// Send necessary info to create the networked entity 
	case Netcode::MessageType::CREATE_NETWORKED_ENTITY:
	{
		TransformComponent* t = e->getComponent<TransformComponent>();
		ArchiveHelpers::archiveVec3(*ar, t->getTranslation()); // Send translation

		// When the remote entity has been created we want to update translation and rotation of that entity
		auto networkComp = e->getComponent<NetworkSenderComponent>();
		networkComp->removeDataType(Netcode::CREATE_NETWORKED_ENTITY);
		networkComp->addDataType(Netcode::MODIFY_TRANSFORM);
		networkComp->addDataType(Netcode::ROTATION_TRANSFORM);
	}
	break;
	case Netcode::MessageType::MODIFY_TRANSFORM:
	{
		TransformComponent* t = e->getComponent<TransformComponent>();
		ArchiveHelpers::archiveVec3(*ar, t->getTranslation()); // Send translation
	}
	break;
	case Netcode::MessageType::ROTATION_TRANSFORM:
	{
		TransformComponent* t = e->getComponent<TransformComponent>();
		ArchiveHelpers::archiveVec3(*ar, t->getRotations());	// Send rotation
	}
	break;
	default:
		break;
	}
}

void NetworkSenderSystem::handleEvent(NetworkSenderEvent* event, cereal::PortableBinaryOutputArchive* ar) {
	{
		(*ar)(event->type); // Send the event-type
	}

	
	switch (event->type) {
	case Netcode::MessageType::SPAWN_PROJECTILE:
	{
		Netcode::MessageDataProjectile* data = static_cast<Netcode::MessageDataProjectile*>(event->data);
		// CURRENTLY IS:
		ArchiveHelpers::archiveVec3(*ar, data->translation);
		ArchiveHelpers::archiveVec3(*ar, data->velocity);
	}
	break;
	case Netcode::MessageType::PLAYER_JUMPED: 
	{
		(*ar)(m_playerEntity->getComponent<LocalOwnerComponent>()->netEntityID);
		// No need to send additional info here, we(this computer) made the jump.
	}
	break;
	case Netcode::MessageType::WATER_HIT_PLAYER:
	{
		Netcode::MessageDataWaterHitPlayer* data = static_cast<Netcode::MessageDataWaterHitPlayer*>(event->data);
		unsigned __int32 NetObjectID = data->playerWhoWasHitID;
		
		(*ar)(NetObjectID);
	}
	break;
	case Netcode::MessageType::PLAYER_DIED:
	{
		Netcode::MessageDataPlayerDied* data = static_cast<Netcode::MessageDataPlayerDied*>(event->data);
		unsigned __int32 NetObjectID = data->playerWhoDied;

		(*ar)(NetObjectID); // Send
	}
	break;
	case Netcode::MessageType::PLAYER_DISCONNECT:
	{
		// NetObjectID should be send outside of this loop.
		Netcode::MessageDataPlayerDisconnect* data = static_cast<Netcode::MessageDataPlayerDisconnect*>(event->data);
		unsigned char NetObjectID = data->playerID;

		(*ar)(NetObjectID); // Send
	}
	break;
	case Netcode::MessageType::MATCH_ENDED:
	{
	}
	break;
	case Netcode::MessageType::SEND_ALL_BACK_TO_LOBBY:
	{
	}
	break;
	case Netcode::MessageType::CANDLE_HELD_STATE:
	{
		Netcode::MessageDataCandleHeldState* data = static_cast<Netcode::MessageDataCandleHeldState*>(event->data);

		__int32 NetObjectID = data->candleOwnerID;
		(*ar)(NetObjectID);
		(*ar)(data->isHeld);
		ArchiveHelpers::archiveVec3(*ar, data->candlePos);
	}
	break;

	default:
		break;
	}
}