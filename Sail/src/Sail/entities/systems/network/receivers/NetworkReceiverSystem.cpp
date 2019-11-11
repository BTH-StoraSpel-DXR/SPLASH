#include "pch.h"
#include "NetworkReceiverSystem.h"
#include "Sail/entities/Entity.h"
#include "Sail/entities/components/NetworkReceiverComponent.h"
#include "Sail/entities/systems/network/NetworkSenderSystem.h"
#include "Sail/entities/components/OnlineOwnerComponent.h"
#include "Sail/entities/components/LocalOwnerComponent.h"
#include "Sail/entities/systems/Gameplay/LevelSystem/LevelSystem.h"
#include "../SPLASH/src/game/states/GameState.h"

#include "Network/NWrapperSingleton.h"
#include "Sail/netcode/ArchiveTypes.h"

#include "Sail/entities/ECS.h"
#include "Sail/entities/systems/physics/UpdateBoundingBoxSystem.h"

// Creation of mid-air bullets from here.
#include "Sail/entities/systems/Gameplay/GunSystem.h"
#include "Sail/utils/GameDataTracker.h"
#include "../SPLASH/src/game/events/GameOverEvent.h"

#include "Sail/events/EventDispatcher.h"

//#define _LOG_TO_FILE
#if defined(DEVELOPMENT) && defined(_LOG_TO_FILE)
#include <fstream>
static std::ofstream out("LogFiles/NetworkReceiverSystem.cpp.log");
#endif

// The host will now automatically forward all incoming messages to other players so
// no need to use any host-specific logic in this system.
#define BANNED(func) sorry_##func##_is_a_banned_function
#undef isHost
#define isHost() BANNED(isHost())


// TODO: register more components
NetworkReceiverSystem::NetworkReceiverSystem() : BaseComponentSystem() {
	registerComponent<NetworkReceiverComponent>(true, true, true);
	registerComponent<TransformComponent>(false, true, true);

	EventDispatcher::Instance().subscribe(Event::Type::NETWORK_DISCONNECT, this);
}

NetworkReceiverSystem::~NetworkReceiverSystem() {
	EventDispatcher::Instance().unsubscribe(Event::Type::NETWORK_DISCONNECT, this);
}

void NetworkReceiverSystem::init(Netcode::PlayerID playerID, GameState* gameStatePtr, NetworkSenderSystem* netSendSysPtr) {
	m_playerID = playerID;
	m_gameStatePtr = gameStatePtr;
	m_netSendSysPtr = netSendSysPtr;

	m_gameDataTracker = &GameDataTracker::getInstance();
}

void NetworkReceiverSystem::pushDataToBuffer(std::string data) {
	std::scoped_lock lock(m_bufferLock);
	m_incomingDataBuffer.push(data);
}

const std::vector<Entity*>& NetworkReceiverSystem::getEntities() const {
	return entities;
}

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
void NetworkReceiverSystem::update(float dt) {
	std::scoped_lock lock(m_bufferLock); // Don't push more data to the buffer whilst this function is running

	size_t nrOfSenderComponents = 0;
	Netcode::PlayerID senderID = 0;
	Netcode::ComponentID id = 0;
	Netcode::MessageType messageType;
	Netcode::EntityType entityType;
	size_t nrOfMessagesInComponent = 0;
	glm::vec3 translation;
	glm::vec3 rotation;
	glm::quat rotationQuat;
	glm::vec3 gunPosition;
	glm::vec3 gunVelocity;
	unsigned int animationIndex;
	float animationTime;

	// Process all messages in the buffer
	while (!m_incomingDataBuffer.empty()) {
		std::istringstream is(m_incomingDataBuffer.front());
		Netcode::InArchive ar(is);

		ar(senderID);
		
		// If the packet was originally sent over the network from ourself 
		// then don't process it and go to the next packet
		if (senderID == m_playerID) { m_incomingDataBuffer.pop(); continue; }

		// If the message was sent internally to ourself then correct the senderID
		if (senderID == Netcode::MESSAGE_FROM_SELF_ID) { senderID = m_playerID; }

		// -+-+-+-+-+-+-+-+ Process data from senderComponents -+-+-+-+-+-+-+-+ 

		ar(nrOfSenderComponents);
		// Read and process data from SenderComponents (i.e. stuff that is continuously updated such as positions)
		for (size_t i = 0; i < nrOfSenderComponents; ++i) {
			ar(id);               // NetworkObject-ID
			ar(entityType);       //
			ar(nrOfMessagesInComponent); //

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
					ar(animationIndex);		// Read
					ar(animationTime);		//
					setEntityAnimation(id, animationIndex, animationTime);
				}
				break;
				case Netcode::MessageType::CHANGE_ABSOLUTE_POS_AND_ROT:
				{
					ArchiveHelpers::loadVec3(ar, translation);
					ArchiveHelpers::loadQuat(ar, rotationQuat);

					setEntityLocalPosition(id, translation);
					setEntityLocalRotation(id, rotationQuat);
				}
				break;
				case Netcode::MessageType::CHANGE_LOCAL_POSITION:
				{
					ArchiveHelpers::loadVec3(ar, translation); // Read translation
					setEntityLocalPosition(id, translation);

				}
				break;
				case Netcode::MessageType::CHANGE_LOCAL_ROTATION:
				{
					ArchiveHelpers::loadVec3(ar, rotation);	// Read rotation
					setEntityLocalRotation(id, rotation);
				}
				break;
				case Netcode::MessageType::SHOOT_START:
				{
					ArchiveHelpers::loadVec3(ar, gunPosition);
					ArchiveHelpers::loadVec3(ar, gunVelocity);

					shootStart(gunPosition, gunVelocity, id);
				}
				break;
				case Netcode::MessageType::SHOOT_LOOP:
				{
					ArchiveHelpers::loadVec3(ar, gunPosition);
					ArchiveHelpers::loadVec3(ar, gunVelocity);

					shootLoop(gunPosition, gunVelocity, id);
				}
				break;
				case Netcode::MessageType::SHOOT_END:
				{
					ArchiveHelpers::loadVec3(ar, gunPosition);
					ArchiveHelpers::loadVec3(ar, gunVelocity);

					shootEnd(gunPosition, gunVelocity, id);
				}
				break;
				default:
					SAIL_LOG_ERROR("INVALID NETWORK MESSAGE RECEIVED FROM " + NWrapperSingleton::getInstance().getPlayer(senderID)->name + "\n");
					break;
				}
			}
		}


		// Receive 'one-time' events
		size_t nrOfEvents;
		Netcode::MessageType eventType = Netcode::MessageType::EMPTY;
		Netcode::MessageType lastEventType = Netcode::MessageType::EMPTY;

#ifdef DEVELOPMENT
		Netcode::MessageType REDUNDANTTYPE;
#endif
		Netcode::ComponentID componentID;
		Netcode::PlayerID playerID;


		// -+-+-+-+-+-+-+-+ Process events -+-+-+-+-+-+-+-+ 
		ar(nrOfEvents);

		// Read and process data from SenderComponents (i.e. stuff that is continuously updated such as positions)
		for (size_t i = 0; i < nrOfEvents; i++) {

			// Handle-Single-Frame events
			ar(eventType);
#ifdef DEVELOPMENT
			ar(REDUNDANTTYPE);
			if (eventType != REDUNDANTTYPE) {
				SAIL_LOG_ERROR("CORRUPTED NETWORK EVENT RECEIVED\n");
				SAIL_LOG_WARNING("Make sure that all players are in either a DEVELOPER branch or the Release branch\n");
				m_incomingDataBuffer.pop();
				return;
			}
#endif
#if defined(DEVELOPMENT) && defined(_LOG_TO_FILE)
			out << "Event: " << Netcode::MessageNames[(int)(eventType) - 1] << "\n";
#endif


			// NOTE: Please keep this switch in alphabetical order (at least for the first word)
			switch (eventType) {
			
			case Netcode::MessageType::CANDLE_HELD_STATE:
			{
				bool isCarried;

				ar(componentID);
				ar(isCarried);

				setCandleHeldState(componentID, isCarried);
			}
			break;
			case Netcode::MessageType::CREATE_NETWORKED_PLAYER:
			{
				Netcode::ComponentID playerCompID;
				Netcode::ComponentID candleCompID;
				Netcode::ComponentID gunCompID;

				ar(playerCompID); // Read what Netcode::ComponentID the player entity should have
				ar(candleCompID); // Read what Netcode::ComponentID the candle entity should have
				ar(gunCompID);    // Read what Netcode::ComponentID the gun entity should have
				ArchiveHelpers::loadVec3(ar, translation); // Read the player's position
				
				createPlayerEntity(playerCompID, candleCompID, gunCompID, translation);
			}
			break;
			case Netcode::MessageType::ENDGAME_STATS:
			{
				// Receive player count
				size_t nrOfPlayers;
				ar(nrOfPlayers);

				// create temporary variables to hold data when reading netmessage
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

				GameDataTracker::getInstance().setStatsForOtherData(bulletsFiredID, bulletsFired, distanceWalkedID, distanceWalked, jumpsMadeID, jumpsMade);

				endMatch();
			}
			break;
			case Netcode::MessageType::EXTINGUISH_CANDLE:
			{
				Netcode::ComponentID candleID;
				Netcode::PlayerID shooterID;

				ar(candleID);
				ar(playerID);

				extinguishCandle(candleID, playerID);
			}
			break;
			case Netcode::MessageType::IGNITE_CANDLE:
			{
				Netcode::ComponentID candleID;
				ar(candleID);
				igniteCandle(candleID);
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
				Netcode::ComponentID networkIdOfKilled;

				ar(networkIdOfKilled); // Receive
				ar(playerIdOfShooter);
				playerDied(networkIdOfKilled, playerIdOfShooter);
			}
			break;
			case Netcode::MessageType::PLAYER_JUMPED:
			{
				ar(componentID);
				playerJumped(componentID);
			}
			break;
			case Netcode::MessageType::PLAYER_LANDED:
			{
				ar(componentID);
				playerLanded(componentID);
			}
			break;
			case Netcode::MessageType::PREPARE_ENDSCREEN:
			{
				GameDataTracker* dgtp = &GameDataTracker::getInstance();
				// create temporary variables to hold data when reading netmessage
				int bulletsFired, jumpsMade;
				float distanceWalked;
				// Get the data
				ar(bulletsFired);
				ar(distanceWalked);
				ar(jumpsMade);

				prepareEndScreen(bulletsFired, distanceWalked, jumpsMade, senderID);
			}
			break;
			case Netcode::MessageType::RUNNING_METAL_START:
			{
				ar(componentID);
				runningMetalStart(componentID);
			}
			break;
			case Netcode::MessageType::RUNNING_TILE_START:
			{
				ar(componentID);
				runningTileStart(componentID);
			}
			break;
			case Netcode::MessageType::RUNNING_STOP_SOUND:
			{
				ar(componentID);
				runningStopSound(componentID);
			}
			break;
			case Netcode::MessageType::SEND_ALL_BACK_TO_LOBBY:
			{
				backToLobby();
			}
			break;
			case Netcode::MessageType::SET_CANDLE_HEALTH: // Only the host will send these messages
			{
				Netcode::ComponentID candleID;
				float health;

				ar(candleID);
				ar(health);

				setCandleHealth(candleID, health);
			}
			break;
			case Netcode::MessageType::SPAWN_PROJECTILE:
			{
				Netcode::ComponentID projectileOwnerID;

				ArchiveHelpers::loadVec3(ar, gunPosition);
				ArchiveHelpers::loadVec3(ar, gunVelocity);
				ar(projectileOwnerID);

				projectileSpawned(gunPosition, gunVelocity, projectileOwnerID);
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
				SAIL_LOG_ERROR("INVALID NETWORK EVENT RECEIVED FROM" + NWrapperSingleton::getInstance().getPlayer(senderID)->name + "\n");
				break;
			}

		}

		m_incomingDataBuffer.pop();
	}

	// End game timer 
	endMatchAfterTimer(dt);
}

/*
  Creates a new entity of the specified entity type and with a NetworkReceiverComponent attached to it
*/
void NetworkReceiverSystem::createPlayerEntity(Netcode::ComponentID playerCompID, Netcode::ComponentID candleCompID, Netcode::ComponentID gunCompID, const glm::vec3& translation) {
	// Early exit if the entity already exists
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == playerCompID) {
			return;
		}
	}

	auto e = ECS::Instance()->createEntity("networkedEntity");
	instantAddEntity(e.get());


	SAIL_LOG("Created player with id: " + std::to_string(playerCompID));

	// lightIndex set to 999, can probably be removed since it no longer seems to be used
	EntityFactory::CreateOtherPlayer(e, playerCompID, candleCompID, gunCompID, 999, translation);

}

// Might need some optimization (like sorting) if we have a lot of networked entities
void NetworkReceiverSystem::setEntityLocalPosition(Netcode::ComponentID id, const glm::vec3& translation) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			e->getComponent<TransformComponent>()->setTranslation(translation);
			return;
		}
	}
	SAIL_LOG_WARNING("setEntityTranslation called but no matching entity found");
}

void NetworkReceiverSystem::setEntityLocalRotation(Netcode::ComponentID id, const glm::quat& rotation) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			e->getComponent<TransformComponent>()->setRotations(rotation);
			return;
		}
	}
	SAIL_LOG_WARNING("setEntityRotation called but no matching entity found");
}

void NetworkReceiverSystem::setEntityLocalRotation(Netcode::ComponentID id, const glm::vec3& rotation) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			e->getComponent<TransformComponent>()->setRotations(rotation);
			return;
		}
	}
	SAIL_LOG_WARNING("setEntityRotation called but no matching entity found");
}

void NetworkReceiverSystem::setEntityAnimation(Netcode::ComponentID id, unsigned int animationIndex, float animationTime) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			auto animation = e->getComponent<AnimationComponent>();
			animation->setAnimation(animationIndex);
			animation->animationTime = animationTime;
			return;
		}
	}
	SAIL_LOG_WARNING("setEntityAnimation called but no matching entity found");
}

void NetworkReceiverSystem::setCandleHealth(Netcode::ComponentID candleId, float health) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == candleId) {
			e->getComponent<CandleComponent>()->health = health;
			return;
		}
	}
	SAIL_LOG_WARNING("setCandleHelath called but no matching candle entity found");
}

void NetworkReceiverSystem::extinguishCandle(Netcode::ComponentID candleId, Netcode::PlayerID shooterID) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == candleId) {

			e->getComponent<CandleComponent>()->wasJustExtinguished = true;
			e->getComponent<CandleComponent>()->wasHitByPlayerID = shooterID;

			return;
		}
	}
	SAIL_LOG_WARNING("extinguishCandle called but no matching candle entity found");
}

void NetworkReceiverSystem::playerJumped(Netcode::ComponentID id) {
	// How do i trigger a jump from here?
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::JUMP].playOnce = true;
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::JUMP].isPlaying = true;

			return;
		}
	}
	SAIL_LOG_WARNING("playerJumped called but no matching entity found");
}

void NetworkReceiverSystem::playerLanded(Netcode::ComponentID id) {

	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::LANDING_GROUND].playOnce = true;
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::LANDING_GROUND].isPlaying = true;

			return;
		}
	}
	SAIL_LOG_WARNING("playerLanded called but no matching entity found");
}


// If I requested the projectile it has a local owner
void NetworkReceiverSystem::projectileSpawned(glm::vec3& pos, glm::vec3 dir, Netcode::ComponentID ownerID) {
	bool wasRequestedByMe = (Netcode::getComponentOwner(ownerID) == m_playerID);

	// Also play the sound
	EntityFactory::CreateProjectile(pos, dir, wasRequestedByMe, ownerID);
}

void NetworkReceiverSystem::playerDied(Netcode::ComponentID networkIdOfKilled, Netcode::PlayerID playerIdOfShooter) {

	Entity* self = nullptr;
	
	// If we are the shooter than we find our entity
	if (m_playerID == playerIdOfShooter) {
		for (auto& e : entities) {
			if (Netcode::getComponentOwner(e->getComponent<NetworkReceiverComponent>()->m_id) == m_playerID) {
				self = e;
				break;
			}
		}
	}

	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id != networkIdOfKilled) {
			continue;
		}

		// Print who killed who
		Netcode::PlayerID idOfDeadPlayer = Netcode::getComponentOwner(networkIdOfKilled);
		std::string deadPlayer = NWrapperSingleton::getInstance().getPlayer(idOfDeadPlayer)->name;
		std::string ShooterPlayer = NWrapperSingleton::getInstance().getPlayer(playerIdOfShooter)->name;
		std::string deathType = "sprayed down";
		SAIL_LOG(ShooterPlayer + " " + deathType + " " + deadPlayer);

		m_gameDataTracker->logPlayerDeath(ShooterPlayer, deadPlayer, deathType);

		//This should remove the candle entity from game
		e->removeDeleteAllChildren();

		// (self == nullptr) == true <--> We are the shooter
		if (self != nullptr) {
			// If it is me who landed the KILLING BLOW
			self->getComponent<AudioComponent>()->m_sounds[Audio::KILLING_BLOW].playOnce = true;
			self->getComponent<AudioComponent>()->m_sounds[Audio::KILLING_BLOW].isPlaying = true;
		}

		// Check if the extinguished candle is owned by the player
		if (Netcode::getComponentOwner(networkIdOfKilled) == m_playerID) {
			//If it is me that died, become spectator.
			e->addComponent<SpectatorComponent>();
			e->getComponent<MovementComponent>()->constantAcceleration = glm::vec3(0.f);
			e->getComponent<MovementComponent>()->velocity = glm::vec3(0.f);
			e->removeComponent<GunComponent>();
			e->removeComponent<AnimationComponent>();
			e->removeComponent<ModelComponent>();
			
			e->getComponent<NetworkSenderComponent>()->removeAllMessageTypes();

			auto transform = e->getComponent<TransformComponent>();
			auto pos = glm::vec3(transform->getCurrentTransformState().m_translation);
			pos.y = 20.f;
			transform->setStartTranslation(pos);
			auto& mapSettings = Application::getInstance()->getSettings().gameSettingsDynamic["map"];
			auto middleOfLevel = glm::vec3(mapSettings["tileSize"].value  * mapSettings["sizeX"].value / 2.f, 0.f, mapSettings["tileSize"].value * mapSettings["sizeY"].value / 2.f);
			auto dir = glm::normalize(middleOfLevel - pos);
			auto rots = Utils::getRotations(dir);
			transform->setRotations(glm::vec3(0.f, -rots.y, rots.x));
		} else {
			//If it wasn't me that died, completely remove the player entity from game.
			e->queueDestruction();
		}

		// Play sound
		e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::DEATH].isPlaying = true;
		e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::DEATH].playOnce = true;
	

		return;
	}
	SAIL_LOG_WARNING("playerDied called but no matching entity found");
}

// NOTE: This is not called on the host, since the host receives the disconnect through NWrapperHost::playerDisconnected()
void NetworkReceiverSystem::playerDisconnect(Netcode::PlayerID playerID) {
	bool found = false;
	for (auto& e : entities) {
		if (Netcode::getComponentOwner(e->getComponent<NetworkReceiverComponent>()->m_id) == playerID) {
			found = true;
			e->removeDeleteAllChildren();
			// TODO: Remove all the components that can/should be removed

			e->queueDestruction();

			return;
		}
	}

	if (!found) {
		SAIL_LOG_WARNING("playerDisconnect called but no matching entity found");
	}
}


// The player who puts down their candle does this in CandleSystem and tests collisions
// The candle will be moved for everyone else in here
void NetworkReceiverSystem::setCandleHeldState(Netcode::ComponentID id, bool isHeld) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id != id) {
			continue;
		}

		for (int i = 0; i < e->getChildEntities().size(); i++) {
			if (auto candleE = e->getChildEntities()[i];  candleE->hasComponent<CandleComponent>()) {
				auto candleComp = candleE->getComponent<CandleComponent>();
				auto candleTransComp = candleE->getComponent<TransformComponent>();


				candleComp->isCarried = isHeld;
				candleComp->wasCarriedLastUpdate = isHeld;
				if (!isHeld) {
					candleTransComp->removeParent();
					e->getComponent<AnimationComponent>()->rightHandEntity = nullptr;

					// Might be needed
					ECS::Instance()->getSystem<UpdateBoundingBoxSystem>()->update(0.0f);
				} else {
					candleTransComp->setTranslation(glm::vec3(10.f, 2.0f, 0.f));
					candleTransComp->setParent(e->getComponent<TransformComponent>());

					e->getComponent<AnimationComponent>()->rightHandEntity = candleE;
				}
				return;
			}
		}
	}
	SAIL_LOG_WARNING("setCandleHeldState called but no matching entity found");
}

void NetworkReceiverSystem::shootStart(glm::vec3& gunPos, glm::vec3& gunVel, Netcode::ComponentID id) {
	// Find out who sent it and make them play the sound (locally)
	for (auto& e : entities) {
		// If we've found who sent the message
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_START].isPlaying = true;
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_START].playOnce = true;
			return;
		}
	}
	SAIL_LOG_WARNING("shootStart called but no matching entity found");
}

void NetworkReceiverSystem::shootLoop(glm::vec3& gunPos, glm::vec3& gunVel, Netcode::ComponentID id) {
	// Find out who sent it and make them play the sound (locally)
	for (auto& e : entities) {
		// If we've found who sent the message
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {

			// Stop Start
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_START].isPlaying = false;

			// Play Loop
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_LOOP].isPlaying = true;
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_LOOP].playOnce = true;
			return;
		}
	}
	SAIL_LOG_WARNING("shootLoop called but no matching entity found");
}

void NetworkReceiverSystem::shootEnd(glm::vec3& gunPos, glm::vec3& gunVel, Netcode::ComponentID id) {
	// Find out who sent it and make them play the sound (locally)
	for (auto& e : entities) {
		// If we've found who sent the message
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			// Stop 
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_LOOP].isPlaying = false;

			// Start the end sound
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_END].isPlaying = true;
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_END].playOnce = true;
			return;
		}
	}
	SAIL_LOG_WARNING("shootEnd called but no matching entity found");
}

void NetworkReceiverSystem::backToLobby() {
	m_gameStatePtr->requestStackPop();
	m_gameStatePtr->requestStackPush(States::JoinLobby);
}

void NetworkReceiverSystem::runningMetalStart(Netcode::ComponentID id) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {

			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::RUN_METAL].isPlaying = true;
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::RUN_METAL].playOnce = false;
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::RUN_TILE].isPlaying = false;

			return;
		}
	}
	SAIL_LOG_WARNING("runningMetalStart called but no matching entity found");
}

void NetworkReceiverSystem::runningTileStart(Netcode::ComponentID id) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {

			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::RUN_TILE].isPlaying = true;
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::RUN_TILE].playOnce = false;
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::RUN_METAL].isPlaying = false;

			return;
		}
	}
	SAIL_LOG_WARNING("runningTileStart called but no matching entity found");
}

void NetworkReceiverSystem::runningStopSound(Netcode::ComponentID id) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {

			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::RUN_METAL].isPlaying = false;
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::RUN_TILE].isPlaying = false;

			return;
		}
	}
	SAIL_LOG_WARNING("runningStopSound called but no matching entity found");
}

bool NetworkReceiverSystem::onEvent(const Event& event) {
	switch (event.type) {
	case Event::Type::NETWORK_DISCONNECT:		playerDisconnect(((const NetworkDisconnectEvent&)(event)).player.id); break;
	default: break;
	}

	return true;
}

void NetworkReceiverSystem::igniteCandle(Netcode::ComponentID candleID) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id != candleID) {
			continue;
		}

		CandleComponent* candle = e->getComponent<CandleComponent>();
		if (!candle->isLit) {
			candle->health = MAX_HEALTH;
			candle->respawns++;
			candle->downTime = 0.f;
			candle->isLit = true;
			candle->userReignition = false;
			candle->invincibleTimer = 1.5f;
		}
		return;
	}
	SAIL_LOG_WARNING("igniteCandle called but no matching entity found");
}
