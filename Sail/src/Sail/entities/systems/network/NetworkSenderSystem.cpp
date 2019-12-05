#include "pch.h"
#include "NetworkSenderSystem.h"

#include "receivers/NetworkReceiverSystem.h"
#include "receivers/KillCamReceiverSystem.h"

#include "Sail/entities/components/NetworkSenderComponent.h"
#include "Sail/entities/components/OnlineOwnerComponent.h"
#include "Sail/entities/components/LocalOwnerComponent.h"
#include "Sail/entities/components/SanityComponent.h"
#include "Sail/entities/Entity.h"

#include "Network/NWrapperSingleton.h"
#include "Sail/../../libraries/cereal/archives/portable_binary.hpp"

#include "../src/Network/NWrapperSingleton.h"
#include "Sail/utils/GameDataTracker.h"

#include <string>
#include <vector>

//#define _LOG_TO_FILE
#if defined(DEVELOPMENT) && defined(_LOG_TO_FILE)
#include <fstream>
static std::ofstream out("LogFiles/NetworkSenderSystem.cpp.log");
#endif

NetworkSenderSystem::NetworkSenderSystem() : BaseComponentSystem() {
	registerComponent<NetworkSenderComponent>(true, true, true);
	registerComponent<TransformComponent>(false, true, false);
}

NetworkSenderSystem::~NetworkSenderSystem() {
	while (!m_eventQueue.empty()) {
		NetworkSenderEvent* pEvent = m_eventQueue.front();
		m_eventQueue.pop();
		delete pEvent;
	}
}

void NetworkSenderSystem::init(Netcode::PlayerID playerID, NetworkReceiverSystem* receiverSystem, KillCamReceiverSystem* killCamSystem) {
	m_playerID = playerID;
	m_receiverSystem = receiverSystem;
	m_killCamSystem = killCamSystem;
}

/*
  The construction of messages needs to match how the NetworkReceiverSystem parses them so
  any changes made here needs to be made there as well!


	Logical structure of the package that will be sent by this function:
	--------------------------------------------------
	| PlayerID        senderID                       |
	| size_t          nrOfEntities                   |
	|     ComponentID     entity[0].id               |
	|     EntityType      entity[0].type             |
	|     size_t          nrOfMessages               |
	|         MessageType     entity[0].messageType  |
	|         MessageData     entity[0].data         |
	|         ...                                    |
	|     ComponentID     entity[1].id               |
	|     EntityType      entity[1].type             |
	|     size_t          nrOfMessages               |
	|         MessageType     entity[0].messageType	 |
	|         MessageData     entity[0].data         |
	|         ...                                    |
	|     ComponentID     entity[2].id               |
	|     EntityType      entity[2].type             |
	|     size_t          nrOfMessages               |
	|         MessageType     entity[0].messageType  |
	|         MessageData     entity[0].data         |
	|         ...                                    |
	|     ...                                        |
	| size_t          nrOfEvents                     |
	|     MessageType     eventType[0]               |
	|     EventData       eventData[0]               |
	|     ...                                        |
	| ...                                            |
	--------------------------------------------------

*/
void NetworkSenderSystem::update() {
	// Binary data that will be sent over the network
	std::ostringstream osToOthers(std::ios::binary);
	Netcode::OutArchive sendToOthers(osToOthers);

	// Binary data that will be sent to our own receiver system so that the network event handling
	// code doesn't need to be duplicated.
	std::ostringstream osToSelf(std::ios::binary);
	Netcode::OutArchive sendToSelf(osToSelf);

	// -+-+-+-+-+-+-+-+ Per-frame sends to per-frame receives via components -+-+-+-+-+-+-+-+ 
	// Send our playerID so that we can ignore this packet when it gets back to us from the host
	sendToOthers(m_playerID);
	sendToSelf(Netcode::MESSAGE_FROM_SELF_ID);


	// See how many SenderComponents have information to send
	size_t nonEmptySenderComponents = 0;
	for (auto e : entities) {
		if (!e->getComponent<NetworkSenderComponent>()->m_dataTypes.empty()) {
			nonEmptySenderComponents++;
		}
	}

	// Write nrOfEntities
	sendToOthers(nonEmptySenderComponents);
	sendToSelf(size_t{ 0 }); // SenderComponent messages should not be sent to ourself

	for (auto e : entities) {
		NetworkSenderComponent* nsc = e->getComponent<NetworkSenderComponent>();

		// If a SenderComponent doesn't have any active messages don't send any of its information
		if (nsc->m_dataTypes.empty()) {
			continue;
		}

		// Create a copy of the message types that are currently in the sender component so that we can make changes to 
		// the sender component without corrupting the packet that we're writing right now.
		const std::vector<Netcode::MessageType> messages = nsc->m_dataTypes;

		sendToOthers(nsc->m_id);         // ComponentID    
		sendToOthers(nsc->m_entityType); // Entity type
		sendToOthers(messages.size());   // NrOfMessages

		// Per type of data
		for (auto& messageType : messages) {
			sendToOthers(messageType);          // Current MessageType
#if defined(DEVELOPMENT) && defined(_LOG_TO_FILE)
			out << "SenderComp: " << Netcode::MessageNames[(int)(messageType) - 1] << "\n";
#endif
			writeMessageToArchive(messageType, e, sendToOthers); // Add to archive depending on the message
		}
	}

	// -+-+-+-+-+-+-+-+ Per-instance events via eventQueue -+-+-+-+-+-+-+-+ 
	sendToOthers(m_eventQueue.size());
	sendToSelf(m_nrOfEventsToSendToSelf.load());

	while (!m_eventQueue.empty()) {
		NetworkSenderEvent* pE = m_eventQueue.front();
#if defined(DEVELOPMENT) && defined(_LOG_TO_FILE)
		out << "Event: " << Netcode::MessageNames[(int)(pE->type)-1] << "\n";
#endif

		writeEventToArchive(pE, sendToOthers);
		if (pE->alsoSendToSelf) {
			writeEventToArchive(pE, sendToSelf);
		}

		m_eventQueue.pop();
		delete pE;
	}
	m_nrOfEventsToSendToSelf = 0;


	// -+-+-+-+-+-+-+-+ send the serialized archive over the network -+-+-+-+-+-+-+-+ 
	std::string binaryDataToSendToOthers = osToOthers.str();
	if (NWrapperSingleton::getInstance().isHost()) {
		NWrapperSingleton::getInstance().getNetworkWrapper()->sendSerializedDataAllClients(binaryDataToSendToOthers);

		// Host doesn't get their messages sent back to them so we need to send them to the killCamReceiverSystem from here
		m_killCamSystem->handleIncomingData(binaryDataToSendToOthers);
	} else {
		NWrapperSingleton::getInstance().getNetworkWrapper()->sendSerializedDataToHost(binaryDataToSendToOthers);
	}


	// -+-+-+-+-+-+-+-+ send Events directly to our own ReceiverSystem -+-+-+-+-+-+-+-+ 
	std::string binaryDataToSendToSelf = osToSelf.str();
	m_receiverSystem->pushDataToBuffer(binaryDataToSendToSelf);



	// -+-+-+-+-+-+-+-+ Host forwards all messages to all clients -+-+-+-+-+-+-+-+ 
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

void NetworkSenderSystem::queueEvent(NetworkSenderEvent* event) {
	std::lock_guard<std::mutex> lock(m_queueMutex);

	m_eventQueue.push(event);

#ifdef DEVELOPMENT
	// Don't send broken events to others or to yourself
	if (event->type < Netcode::MessageType::DESTROY_ENTITY || event->type >= Netcode::MessageType::EMPTY) {
		SAIL_LOG_ERROR("Attempted to send invalid message\n");
		return;
	}
#endif

	// if the event will be sent to ourself then increment the size counter
	if (event->alsoSendToSelf) {
		m_nrOfEventsToSendToSelf++;
	}
}


// ONLY DO FOR THE HOST
// Push incoming data strings to the back of a FIFO list which will be forwarded to all other players
void NetworkSenderSystem::pushDataToBuffer(const std::string& data) {
	std::lock_guard<std::mutex> lock(m_forwardBufferLock);
	m_HOSTONLY_dataToForward.push(data);
}

#ifdef DEVELOPMENT
unsigned int NetworkSenderSystem::getByteSize() const {
	unsigned int size = BaseComponentSystem::getByteSize() + sizeof(*this);
	size += m_eventQueue.size() * sizeof(NetworkSenderEvent*);
	const size_t queueSize = m_HOSTONLY_dataToForward.size();
	size += queueSize * sizeof(std::string);
	if (queueSize) {
		size += queueSize * m_HOSTONLY_dataToForward.front().capacity() * sizeof(unsigned char);		// Approximate string length
	}
	return size;
}
#endif

// TODO: Test this to see if it's actually needed or not
void NetworkSenderSystem::stop() {
	// Loop through networked entities and serialize their data.
	std::ostringstream osToOthers(std::ios::binary);
	Netcode::OutArchive sendToOthers(osToOthers);

	sendToOthers(m_playerID);
	sendToOthers(size_t{0}); // Write nrOfEntities

	// -+-+-+-+-+-+-+-+ Per-instance events via eventQueue -+-+-+-+-+-+-+-+ 
	bool ended = false;

	while (!m_eventQueue.empty()) {
		NetworkSenderEvent* pE = m_eventQueue.front();		// Fetch
		if ((pE->type == Netcode::MessageType::MATCH_ENDED) && ended == false) {
			ended = true;
			sendToOthers(size_t{1}); // Write nrOfEvents
			writeEventToArchive(pE, sendToOthers);
		}
		m_eventQueue.pop();
		delete pE;
	}


	if (ended) {
		// send the serialized archive over the network
		std::string binaryData = osToOthers.str();
		if (NWrapperSingleton::getInstance().isHost()) {
			NWrapperSingleton::getInstance().getNetworkWrapper()->sendSerializedDataAllClients(binaryData);
		} else {
			NWrapperSingleton::getInstance().getNetworkWrapper()->sendSerializedDataToHost(binaryData);
		}
	}
}

void NetworkSenderSystem::writeMessageToArchive(const Netcode::MessageType& messageType, Entity* e, Netcode::OutArchive& ar) {
	// Package it depending on the type
	// NOTE: Please keep this switch in alphabetical order (at least for the first word)
	switch (messageType) {
		// Send necessary info to create the networked entity 
	case Netcode::MessageType::ANIMATION:
	{
		ar(e->getComponent<AnimationComponent>()->animationIndex);
		ar(e->getComponent<AnimationComponent>()->animationTime);


		ar(e->getComponent<AnimationComponent>()->pitch);
	}
	break; 
	case Netcode::MessageType::CHANGE_ABSOLUTE_POS_AND_ROT:
	{
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;

		TransformComponent* t = e->getComponent<TransformComponent>();

		glm::decompose(t->getMatrixWithUpdate(), scale, rotation, translation, skew, perspective);

		ArchiveHelpers::saveVec3(ar, translation);
		ArchiveHelpers::saveQuat(ar, rotation);
	}
	break;
	case Netcode::MessageType::CHANGE_LOCAL_POSITION:
	{
		TransformComponent* t = e->getComponent<TransformComponent>();
		ArchiveHelpers::saveVec3(ar, t->getTranslation());
	}
	break;
	case Netcode::MessageType::CHANGE_LOCAL_ROTATION:
	{
		TransformComponent* t = e->getComponent<TransformComponent>();
		ArchiveHelpers::saveVec3(ar, t->getRotations());
	}
	break;
	case Netcode::MessageType::DESTROY_ENTITY:
	{
		e->getComponent<NetworkSenderComponent>()->removeAllMessageTypes();
	}
	break;
	case Netcode::MessageType::SHOOT_START:
	{
		// Only do this once
		e->getComponent<NetworkSenderComponent>()->removeMessageType(Netcode::MessageType::SHOOT_START);

		// Send data to others
		ar(e->getComponent<AudioComponent>()->m_sounds[Audio::SHOOT_START].frequency);

		// Transition into loop
		e->getComponent<NetworkSenderComponent>()->addMessageType(Netcode::MessageType::SHOOT_LOOP);
	}
	break;
	case Netcode::MessageType::SHOOT_LOOP:
	{
		// Send data to others
		ar(e->getComponent<AudioComponent>()->m_sounds[Audio::SHOOT_LOOP].frequency);
	}
	break;
	case Netcode::MessageType::SHOOT_END:
	{
		// Only do this once
		e->getComponent<NetworkSenderComponent>()->removeMessageType(Netcode::MessageType::SHOOT_END);
		// Stop looping
		e->getComponent<NetworkSenderComponent>()->removeMessageType(Netcode::MessageType::SHOOT_LOOP);

		// Send data to others
		ar(e->getComponent<AudioComponent>()->m_sounds[Audio::SHOOT_END].frequency);
	}
	break;
	case Netcode::MessageType::UPDATE_SANITY:
	{
		SanityComponent* ic = e->getComponent<SanityComponent>();
		ar(ic->sanity);
	}
	break;
	case Netcode::MessageType::UPDATE_PROJECTILE_ONCE:
	{
		TransformComponent* tc = e->getComponent<TransformComponent>();
		MovementComponent* mc = e->getComponent<MovementComponent>();
		
		ArchiveHelpers::saveVec3(ar, tc->getCurrentTransformState().m_translation);
		ArchiveHelpers::saveVec3(ar, mc->velocity);

		// Only send this message when their is a collision
		e->getComponent<NetworkSenderComponent>()->removeMessageType(Netcode::MessageType::UPDATE_PROJECTILE_ONCE);
	}
	break;
	default:
		SAIL_LOG_ERROR("TRIED TO SEND INVALID NETWORK MESSAGE (" + std::to_string((int)messageType));
		break;
	}
}

void NetworkSenderSystem::writeEventToArchive(NetworkSenderEvent* event, Netcode::OutArchive& ar) {
	ar(event->type); // Send the event-type

	// NOTE: Please keep this switch in alphabetical order (at least for the first word)
	switch (event->type) {
	case Netcode::MessageType::CANDLE_HELD_STATE:
	{
		Netcode::MessageCandleHeldState* data = static_cast<Netcode::MessageCandleHeldState*>(event->data);

		ar(data->candleOwnerID);
		ar(data->isHeld);
	}
	break;
	case Netcode::MessageType::ENABLE_SPRINKLERS:
	{
		Netcode::MessageHitBySprinkler* data = static_cast<Netcode::MessageHitBySprinkler*>(event->data);
	}
	break;
	case Netcode::MessageType::ENDGAME_STATS:
	{
		GameDataTracker* dgtp = &GameDataTracker::getInstance();
		const std::map<Netcode::PlayerID, HostStatsPerPlayer> tmpPlayerMap = dgtp->getPlayerDataMap();
		// Send player count to clients for them to loop following data
		ar(tmpPlayerMap.size());

		// Send all per player data. Match this on the reciever end
		for (auto player = tmpPlayerMap.begin(); player != tmpPlayerMap.end(); ++player) {
			ar(player->first);
			ar(player->second.placement);
			ar(player->second.nKills);
			ar(player->second.nDeaths);
			ar(player->second.damage);
			ar(player->second.damageTaken);
		}

		// Send all specific data. The host has processed data from all clients and will 
		// now return it to their endscreens.
		ar(dgtp->getStatisticsGlobal().bulletsFired);
		ar(dgtp->getStatisticsGlobal().bulletsFiredID);

		ar(dgtp->getStatisticsGlobal().distanceWalked);
		ar(dgtp->getStatisticsGlobal().distanceWalkedID);

		ar(dgtp->getStatisticsGlobal().jumpsMade);
		ar(dgtp->getStatisticsGlobal().jumpsMadeID);


	}
	break;
	case Netcode::MessageType::EXTINGUISH_CANDLE:
	{
		Netcode::MessageExtinguishCandle* data = static_cast<Netcode::MessageExtinguishCandle*>(event->data);
		ar(data->candleThatWasHit);
		ar(data->playerWhoExtinguishedCandle);
	}
	break;
	case Netcode::MessageType::HIT_BY_SPRINKLER:
	{
		Netcode::MessageHitBySprinkler* data = static_cast<Netcode::MessageHitBySprinkler*>(event->data);

		ar(data->candleOwnerID);
	}
	break;
	case Netcode::MessageType::IGNITE_CANDLE:
	{
		Netcode::MessageIgniteCandle* data = static_cast<Netcode::MessageIgniteCandle*>(event->data);
		ar(data->candleCompId);
	}
	break;
	case Netcode::MessageType::MATCH_ENDED:
	{

	}
	break;
	case Netcode::MessageType::PLAYER_DIED:
	{
		Netcode::MessagePlayerDied* data = static_cast<Netcode::MessagePlayerDied*>(event->data);

		ar(data->playerWhoDied); // Send
		ar(data->killingEntity);
		ar(data->isFinalKill);
	}
	break;
	case Netcode::MessageType::PLAYER_JUMPED:
	{
		Netcode::MessagePlayerJumped* data = static_cast<Netcode::MessagePlayerJumped*>(event->data);
		ar(data->playerWhoJumped);
	}
	break;
	case Netcode::MessageType::PLAYER_LANDED:
	{
		Netcode::MessagePlayerLanded* data = static_cast<Netcode::MessagePlayerLanded*>(event->data);

		ar(data->playerWhoLanded);
	}
	break;
	case Netcode::MessageType::PREPARE_ENDSCREEN:
	{
		// Send all specific data to Host
		ar(GameDataTracker::getInstance().getStatisticsLocal().bulletsFired);
		ar(GameDataTracker::getInstance().getStatisticsLocal().distanceWalked);
		ar(GameDataTracker::getInstance().getStatisticsLocal().jumpsMade);
	}
	break;
	case Netcode::MessageType::RUNNING_METAL_START:
	{
		Netcode::MessageRunningMetalStart* data = static_cast<Netcode::MessageRunningMetalStart*>(event->data);
		ar(data->runningPlayer); // Send
	}
	break;
	case Netcode::MessageType::RUNNING_TILE_START:
	{
		Netcode::MessageRunningTileStart* data = static_cast<Netcode::MessageRunningTileStart*>(event->data);
		ar(data->runningPlayer); // Send
	}
	break;
	case Netcode::MessageType::RUNNING_WATER_METAL_START:
	{
		Netcode::MessageRunningWaterMetalStart* data = static_cast<Netcode::MessageRunningWaterMetalStart*>(event->data);
		ar(data->runningPlayer);
	}
	break;
	case Netcode::MessageType::RUNNING_WATER_TILE_START:
	{
		Netcode::MessageRunningWaterTileStart* data = static_cast<Netcode::MessageRunningWaterTileStart*>(event->data);
		ar(data->runningPlayer);
	}
	break;
	case Netcode::MessageType::RUNNING_STOP_SOUND:
	{
		Netcode::MessageRunningStopSound* data = static_cast<Netcode::MessageRunningStopSound*>(event->data);
		ar(data->runningPlayer); // Send
	}
	break;
	case Netcode::MessageType::SET_CANDLE_HEALTH:
	{
		Netcode::MessageSetCandleHealth* data = static_cast<Netcode::MessageSetCandleHealth*>(event->data);
		
		ar(data->candleThatWasHit);
		ar(data->health);
	}
	break;
	case Netcode::MessageType::SUBMIT_WATER_POINTS:
	{
		Netcode::MessageSubmitWaterPoints* data = static_cast<Netcode::MessageSubmitWaterPoints*>(event->data);
		
		size_t nrOfPoints = data->points.size();

		ar(nrOfPoints); // Tell the receiver how many points they should receive

		for (size_t i = 0; i < nrOfPoints; i++) {
			ArchiveHelpers::saveVec3(ar, data->points[i]); // Send all points to the receiver
		}
	}
	break;
	case Netcode::MessageType::SPAWN_PROJECTILE:
	{
		Netcode::MessageSpawnProjectile* data = static_cast<Netcode::MessageSpawnProjectile*>(event->data);

		ArchiveHelpers::saveVec3(ar, data->translation);
		ArchiveHelpers::saveVec3(ar, data->velocity);
		ar(data->projectileComponentID);
		ar(data->ownerPlayerComponentID);
	}
	break;
	case Netcode::MessageType::START_THROWING:
	{
		Netcode::MessageStartThrowing* data = static_cast<Netcode::MessageStartThrowing*>(event->data);
		ar(data->playerCompID); // Send
	}
	break;
	case Netcode::MessageType::STOP_THROWING:
	{
		Netcode::MessageStopThrowing* data = static_cast<Netcode::MessageStopThrowing*>(event->data);
		ar(data->playerCompID); // Send
	}
	break;
	case Netcode::MessageType::WATER_HIT_PLAYER:
	{
		Netcode::MessageWaterHitPlayer* data = static_cast<Netcode::MessageWaterHitPlayer*>(event->data);

		ar(data->playerWhoWasHitID);
		ar(data->projectileThatHitID);
	}
	break;
	default:
		SAIL_LOG_ERROR("TRIED TO SEND INVALID NETWORK EVENT (" + std::to_string((int)event->type));
		break;
	}
}