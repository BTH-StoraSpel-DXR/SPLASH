
#include "pch.h"
#include "ReceiverBase.h"
#include "Sail/entities/Entity.h"

#include "Sail/utils/GameDataTracker.h"
#include "Sail/utils/Utils.h"

#include "Network/NWrapperSingleton.h"

#include "../SPLASH/src/game/events/GameOverEvent.h"

ReceiverBase::ReceiverBase() : BaseComponentSystem()
{}

ReceiverBase::~ReceiverBase() 
{}

void ReceiverBase::init(Netcode::PlayerID playerID, NetworkSenderSystem* NSS) {
	m_playerID = playerID;
	m_netSendSysPtr = NSS;
	m_gameDataTracker = &GameDataTracker::getInstance();
}

void ReceiverBase::setPlayer   (Entity* player)       { m_playerEntity = player; }
void ReceiverBase::setGameState(GameState* gameState) { m_gameStatePtr = gameState; }

const std::vector<Entity*>& ReceiverBase::getEntities() const { return entities; }


/*
  The parsing of messages needs to match how the NetworkSenderSystem constructs them so
  any changes made here needs to be made there as well!

  Logical structure of the packages that will be decoded by this function:
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
void ReceiverBase::processData(float dt, std::queue<std::string>& data) {
	// TODO: Remove a bunch of stuff from here
	size_t nrOfSenderComponents    = 0;
	size_t nrOfMessagesInComponent = 0;
	size_t nrOfEventsInPacket      = 0;
	
	Netcode::PlayerID    senderID    = 0; // The playerID of the person who sent the message
	Netcode::EntityType  entityType  = Netcode::EntityType::INVALID_ENTITY; // Entity that sent a message
	Netcode::MessageType messageType = Netcode::MessageType::EMPTY;
	Netcode::ComponentID compID = 0;

	// Commonly used types within messages/events:
	glm::vec3 vector;
	glm::quat quaternion;


	// Process all messages in the buffer
	while (!data.empty()) {
		std::istringstream is(data.front());
		Netcode::InArchive ar(is);

		ar(senderID);

		// If the packet was originally sent over the network from ourself 
		// then don't process it and go to the next packet
		if (senderID == m_playerID) { data.pop(); continue; }

		// If the message was sent internally to ourself then correct the senderID
		if (senderID == Netcode::MESSAGE_FROM_SELF_ID) { senderID = m_playerID; }

		// -+-+-+-+-+-+-+-+ Process data from senderComponents -+-+-+-+-+-+-+-+ 

		ar(nrOfSenderComponents);
		// Read and process data from SenderComponents (i.e. stuff that is continuously updated such as positions)
		for (size_t i = 0; i < nrOfSenderComponents; ++i) {
			ar(compID);
			ar(entityType);
			ar(nrOfMessagesInComponent);

			// Read per data type
			for (size_t j = 0; j < nrOfMessagesInComponent; j++) {
				ar(messageType);

#if defined(DEVELOPMENT) && defined(_LOG_TO_FILE)
				out << "ReciverComp: " << Netcode::MessageNames[(int)(messageType)-1] << "\n";
#endif

				// Read and process the data
				// NOTE: Please keep this switch in alphabetical order (at least for the first word)
				switch (messageType) {
				case Netcode::MessageType::ANIMATION:
				{
					AnimationInfo info;
					ar(info.index);
					ar(info.time);
					setAnimation(compID, info);
				}
				break;
				case Netcode::MessageType::CHANGE_ABSOLUTE_POS_AND_ROT:
				{
					ArchiveHelpers::loadVec3(ar, vector);
					ArchiveHelpers::loadQuat(ar, quaternion);

					setLocalPosition(compID, vector);
					setLocalRotation(compID, quaternion);
				}
				break;
				case Netcode::MessageType::CHANGE_LOCAL_POSITION:
				{
					ArchiveHelpers::loadVec3(ar, vector); // Read translation
					setLocalPosition(compID, vector);
				}
				break;
				case Netcode::MessageType::CHANGE_LOCAL_ROTATION:
				{
					ArchiveHelpers::loadVec3(ar, vector);	// Read rotation
					setLocalRotation(compID, vector);
				}
				break;
				case Netcode::MessageType::DESTROY_ENTITY:
				{
					destroyEntity(compID);
				}
				break;
				case Netcode::MessageType::SHOOT_START:
				{
					ShotFiredInfo info;

					ArchiveHelpers::loadVec3(ar, info.gunPosition);
					ArchiveHelpers::loadVec3(ar, info.gunVelocity);

					shootStart(compID, info);
				}
				break;
				case Netcode::MessageType::SHOOT_LOOP:
				{
					ShotFiredInfo info;

					ArchiveHelpers::loadVec3(ar, info.gunPosition);
					ArchiveHelpers::loadVec3(ar, info.gunVelocity);

					shootLoop(compID, info);
				}
				break;
				case Netcode::MessageType::SHOOT_END:
				{
					ShotFiredInfo info;

					ArchiveHelpers::loadVec3(ar, info.gunPosition);
					ArchiveHelpers::loadVec3(ar, info.gunVelocity);

					shootEnd(compID, info);
				}
				break;
				default:
					SAIL_LOG_ERROR("INVALID NETWORK MESSAGE RECEIVED FROM " + NWrapperSingleton::getInstance().getPlayer(senderID)->name + "\n");
					break;
				}
			}
		}


		// Receive 'one-time' events
		// -+-+-+-+-+-+-+-+ Process events -+-+-+-+-+-+-+-+ 
		ar(nrOfEventsInPacket);

		// Read and process data from SenderComponents (i.e. stuff that is continuously updated such as positions)
		for (size_t i = 0; i < nrOfEventsInPacket; ++i) {

			// Handle-Single-Frame events
			ar(messageType);

#if defined(DEVELOPMENT) && defined(_LOG_TO_FILE)
			out << "Event: " << Netcode::MessageNames[(int)(eventType)-1] << "\n";
#endif


			// NOTE: Please keep this switch in alphabetical order (at least for the first word)
			switch (messageType) {

			case Netcode::MessageType::CANDLE_HELD_STATE:
			{
				bool isCarried;

				ar(compID);
				ar(isCarried);

				setCandleState(compID, isCarried);
			}
			break;
			case Netcode::MessageType::CREATE_NETWORKED_PLAYER:
			{
				PlayerComponentInfo info;

				ar(info.playerCompID); // Read what Netcode::ComponentID the player entity should have
				ar(info.candleID);     // Read what Netcode::ComponentID the candle entity should have
				ar(info.gunID);        // Read what Netcode::ComponentID the gun entity should have
				ArchiveHelpers::loadVec3(ar, vector); // Read the player's position

				createPlayer(info, vector);
			}
			break;
			case Netcode::MessageType::ENABLE_SPRINKLERS:
			{
				enableSprinklers();
			}
			break;
			case Netcode::MessageType::ENDGAME_STATS:
			{
				// Receive player count
				size_t nrOfPlayers;
				ar(nrOfPlayers);

				// create temporary variables to hold data when reading message
				Netcode::PlayerID pID;
				int nKills;
				int placement;

				int bulletsFired, jumpsMade;
				float distanceWalked;
				Netcode::PlayerID bulletsFiredID, distanceWalkedID, jumpsMadeID;

				// Get all per player data from the Host
				for (int k = 0; k < nrOfPlayers; k++) {
					ar(pID);
					ar(nKills);
					ar(placement);
					GameDataTracker::getInstance().setStatsForPlayer(pID, nKills, placement);
				}

				// Get all specific data from the Host
				ar(bulletsFired);
				ar(bulletsFiredID);

				ar(distanceWalked);
				ar(distanceWalkedID);

				ar(jumpsMade);
				ar(jumpsMadeID);

				GameDataTracker::getInstance().setStatsForOtherData(
					bulletsFiredID, bulletsFired, distanceWalkedID, distanceWalked, jumpsMadeID, jumpsMade);

				endMatch();
			}
			break;
			case Netcode::MessageType::EXTINGUISH_CANDLE:
			{
				Netcode::PlayerID shooterID;

				ar(compID);
				ar(shooterID);

				extinguishCandle(compID, shooterID);
			}
			break;
			case Netcode::MessageType::HIT_BY_SPRINKLER:
			{
				ar(compID);

				hitBySprinkler(compID);
			}
			break;
			case Netcode::MessageType::IGNITE_CANDLE:
			{
				ar(compID);

				igniteCandle(compID);
			}
			break;
			case Netcode::MessageType::MATCH_ENDED:
			{
				NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
					Netcode::MessageType::PREPARE_ENDSCREEN,
					SAIL_NEW Netcode::MessagePrepareEndScreen(),
					false
				);

				GameDataTracker::getInstance().turnOffLocalDataTracking();

				mergeHostsStats();
				// Dispatch game over event
				EventDispatcher::Instance().emit(GameOverEvent());
			}
			break;
			case Netcode::MessageType::PLAYER_DIED:
			{
				Netcode::PlayerID playerIdOfShooter;

				ar(compID);
				ar(playerIdOfShooter);
				
				playerDied(compID, playerIdOfShooter);
			}
			break;
			case Netcode::MessageType::PLAYER_JUMPED:
			{
				ar(compID);
				playerJumped(compID);
			}
			break;
			case Netcode::MessageType::PLAYER_LANDED:
			{
				ar(compID);
				playerLanded(compID);
			}
			break;
			case Netcode::MessageType::PREPARE_ENDSCREEN:
			{
				GameDataTracker* dgtp = &GameDataTracker::getInstance();
				EndScreenInfo info;

				// Get the data
				ar(info.bulletsFired);
				ar(info.distanceWalked);
				ar(info.jumpsMade);

				prepareEndScreen(senderID, info);
			}
			break;
			case Netcode::MessageType::RUNNING_METAL_START:
			{
				ar(compID);
				runningMetalStart(compID);
			}
			break;
			case Netcode::MessageType::RUNNING_WATER_METAL_START:
			{
				ar(compID);
				runningWaterMetalStart(compID);
			}
			break;
			case Netcode::MessageType::RUNNING_TILE_START:
			{
				ar(compID);
				runningTileStart(compID);
			}
			break;
			case Netcode::MessageType::RUNNING_WATER_TILE_START:
			{
				ar(compID);
				runningWaterTileStart(compID);
			}
			break;
			case Netcode::MessageType::RUNNING_STOP_SOUND:
			{
				ar(compID);
				runningStopSound(compID);
			}
			break;
			case Netcode::MessageType::SET_CANDLE_HEALTH:
			{
				float health;

				ar(compID);
				ar(health);

				setCandleHealth(compID, health);
			}
			break;
			case Netcode::MessageType::SPAWN_PROJECTILE:
			{
				ProjectileInfo info;

				ArchiveHelpers::loadVec3(ar, info.position);
				ArchiveHelpers::loadVec3(ar, info.velocity);
				ar(info.projectileID);
				ar(info.ownerID);

				spawnProjectile(info);
			}
			break;
			case Netcode::MessageType::WATER_HIT_PLAYER:
			{
				Netcode::ComponentID playerwhoWasHit;
				ar(playerwhoWasHit);

				// NOTE!
				// This function is and should be empty for the NetworkReceiverSystemClient. 
				// Only the Host has the authority to damage candles.
				waterHitPlayer(playerwhoWasHit, senderID);
			}
			break;
			default:
				SAIL_LOG_ERROR("INVALID NETWORK EVENT NR " + std::to_string((int)messageType) + " RECEIVED FROM" + NWrapperSingleton::getInstance().getPlayer(senderID)->name + "\n");
				break;
			}
		}

		data.pop();
	}

	// End game timer 
	endMatchAfterTimer(dt);
}










