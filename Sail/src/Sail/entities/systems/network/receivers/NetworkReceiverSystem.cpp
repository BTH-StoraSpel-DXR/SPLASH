#include "pch.h"
#include "NetworkReceiverSystem.h"
#include "Sail/entities/Entity.h"
#include "Sail/entities/components/NetworkReceiverComponent.h"
#include "Sail/entities/systems/network/NetworkSenderSystem.h"
#include "Sail/entities/components/OnlineOwnerComponent.h"
#include "Sail/entities/components/LocalOwnerComponent.h"
#include "Sail/entities/components/MapComponent.h"
#include "../SPLASH/src/game/states/GameState.h"

#include "Network/NWrapperSingleton.h"
#include "Sail/netcode/ArchiveTypes.h"

#include "Sail/entities/ECS.h"
#include "Sail/entities/systems/physics/UpdateBoundingBoxSystem.h"

// Creation of mid-air bullets from here.
#include "Sail/entities/systems/Gameplay/GunSystem.h"
#include "Sail/utils/GameDataTracker.h"


// The host will now automatically forward all incoming messages to other players so
// no need to use any host-specific logic in this system.
#define BANNED(func) sorry_##func##_is_a_banned_function
#undef isHost
#define isHost() BANNED(isHost())


// TODO: register more components
NetworkReceiverSystem::NetworkReceiverSystem() : BaseComponentSystem() {
	registerComponent<NetworkReceiverComponent>(true, true, true);
	registerComponent<TransformComponent>(false, true, true);
}

NetworkReceiverSystem::~NetworkReceiverSystem() {
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
void NetworkReceiverSystem::update() {
	std::scoped_lock lock(m_bufferLock); // Don't push more data to the buffer whilst this function is running

	size_t nrOfSenderComponents = 0;
	Netcode::PlayerID senderID = 0;
	Netcode::ComponentID id = 0;
	Netcode::MessageType messageType;
	Netcode::EntityType entityType;
	size_t nrOfMessagesInComponent = 0;
	glm::vec3 translation;
	glm::vec3 rotation;
	glm::vec3 gunPosition;
	glm::vec3 gunVelocity;
	int animationStack;
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
			ar(id);               // Component-ID
			ar(entityType);       //
			ar(nrOfMessagesInComponent); //

			// Read per data type
			for (size_t j = 0; j < nrOfMessagesInComponent; j++) {
				ar(messageType);

				// Read and process the data
				// TODO: Rename some of the enums/functions
				switch (messageType) {
				case Netcode::MessageType::CREATE_NETWORKED_ENTITY:
				{
					ArchiveHelpers::loadVec3(ar, translation); // Read translation
					createEntity(id, entityType, translation);
				}
				break;
				case Netcode::MessageType::MODIFY_TRANSFORM:
				{
					ArchiveHelpers::loadVec3(ar, translation); // Read translation
					setEntityTranslation(id, translation);

				}
				break;
				case Netcode::MessageType::ROTATION_TRANSFORM:
				{
					ArchiveHelpers::loadVec3(ar, rotation);	// Read rotation
					setEntityRotation(id, rotation);
				}
				break;
				case Netcode::MessageType::ANIMATION: 
				{
					ar(animationStack);		// Read
					ar(animationTime);		//
					setEntityAnimation(id, animationStack, animationTime);
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
					break;
				}
			}
		}


		// Receive 'one-time' events
		size_t nrOfEvents;
		Netcode::MessageType eventType;
		Netcode::ComponentID componentID;


		// -+-+-+-+-+-+-+-+ Process events -+-+-+-+-+-+-+-+ 
		ar(nrOfEvents);
		// Read and process data from SenderComponents (i.e. stuff that is continuously updated such as positions)
		for (size_t i = 0; i < nrOfEvents; i++) {
			// Handle-Single-Frame events
			ar(eventType);

			switch (eventType) {
			case Netcode::MessageType::PLAYER_JUMPED:
			{
				ar(componentID);
				playerJumped(componentID);
			}
			break;
			case Netcode::MessageType::WATER_HIT_PLAYER:
			{
				Netcode::ComponentID playerwhoWasHit;
				ar(playerwhoWasHit);
				waterHitPlayer(playerwhoWasHit, senderID);
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
			case Netcode::MessageType::PLAYER_DIED:
			{
				Netcode::PlayerID playerIdOfShooter;
				Netcode::ComponentID networkIdOfKilled;

				ar(networkIdOfKilled); // Receive
				ar(playerIdOfShooter);
				playerDied(networkIdOfKilled, playerIdOfShooter);
			}
			break;
			case Netcode::MessageType::MATCH_ENDED:
			{
				matchEnded();
			}
			break;
			case Netcode::MessageType::CANDLE_HELD_STATE:
			{
				glm::vec3 candlepos;
				bool isCarried;

				ar(componentID);
				ar(isCarried);
				ArchiveHelpers::loadVec3(ar, candlepos);
				
				setCandleHeldState(componentID, isCarried, candlepos);
			}
			break;
			case Netcode::MessageType::SEND_ALL_BACK_TO_LOBBY:
			{
				backToLobby();
			}
			break;
			case Netcode::MessageType::PLAYER_DISCONNECT:
			{
				Netcode::PlayerID playerID;

				ar(playerID);
				playerDisconnect(playerID);
			}
			break;
			default:
				break;
			}

		}

		m_incomingDataBuffer.pop();
	}
}

/*
  Creates a new entity of the specified entity type and with a NetworkReceiverComponent attached to it
*/
void NetworkReceiverSystem::createEntity(Netcode::ComponentID id, Netcode::EntityType entityType, const glm::vec3& translation) {
	// Early exit if the entity already exists
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			return;
		}
	}

	auto e = ECS::Instance()->createEntity("networkedEntity");
	entities.push_back(e.get());

	// create the new entity
	switch (entityType) {
	case Netcode::EntityType::PLAYER_ENTITY:
	{
		// lightIndex set to 999, can probably be removed since it no longer seems to be used
		EntityFactory::CreateOtherPlayer(e, id, 999, translation);
	}
	break;
	default:
		break;
	}
}

// Might need some optimization (like sorting) if we have a lot of networked entities
void NetworkReceiverSystem::setEntityTranslation(Netcode::ComponentID id, const glm::vec3& translation) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			e->getComponent<TransformComponent>()->setTranslation(translation);
			return;
		}
	}
	Logger::Warning("setEntityTranslation called but no matching entity found");
}

void NetworkReceiverSystem::setEntityRotation(Netcode::ComponentID id, const glm::vec3& rotation) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			//TODO: REMOVE THIS WHEN NEW ANIMATIONS ARE PUT IN
			//TODO: REMOVE
			//TODO: REMOVE	//TODO: REMOVE THIS WHEN NEW ANIMATIONS ARE PUT IN
			glm::vec3 rot = rotation;
			//if (e->getComponent<AnimationComponent>()->currentAnimation != e->getComponent<AnimationComponent>()->getAnimationStack()->getAnimation(0)) {
			rot.y += 3.14f * 0.5f;
			//}
			e->getComponent<TransformComponent>()->setRotations(rot);

			return;
		}
	}
	Logger::Warning("setEntityRotation called but no matching entity found");
}

void NetworkReceiverSystem::setEntityAnimation(Netcode::ComponentID id, int animationStack, float animationTime) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			auto animation = e->getComponent<AnimationComponent>();
			animation->currentAnimation = animation->getAnimationStack()->getAnimation(animationStack);
			animation->animationTime = animationTime;
			return;
		}
	}
	Logger::Warning("setEntityAnimation called but no matching entity found");
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
	Logger::Warning("playerJumped called but no matching entity found");
}

void NetworkReceiverSystem::waterHitPlayer(Netcode::ComponentID id, Netcode::PlayerID senderId) {
	for (auto& e : entities) {
		//Look for the entity that OWNS the candle (player entity)
		if (e->getComponent<NetworkReceiverComponent>()->m_id != id) {
			continue;
		}
		//Look for the entity that IS the candle (candle entity)
		std::vector<Entity::SPtr> childEntities = e->getChildEntities();
		for (auto& child : childEntities) {
			if (child->hasComponent<CandleComponent>()) {
				// Damage the candle
				// Save the Shooter of the Candle if its lethal
				child->getComponent<CandleComponent>()->hitWithWater(10.0f, senderId);
				// Check in Candle System What happens next
				return;
			}
		}
	}
	Logger::Warning("waterHitPlayer called but no matching entity found");
}


// If I requested the projectile it has a local owner
void NetworkReceiverSystem::projectileSpawned(glm::vec3& pos, glm::vec3 dir, Netcode::ComponentID ownerID) {
	bool wasRequestedByMe = (Netcode::getComponentOwner(ownerID) == m_playerID);

	// Also play the sound
	EntityFactory::CreateProjectile(pos, dir, wasRequestedByMe, ownerID);
}

void NetworkReceiverSystem::playerDied(Netcode::ComponentID networkIdOfKilled, Netcode::PlayerID playerIdOfShooter) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id != networkIdOfKilled) {
			continue;
		}

		// Print who killed who
		Netcode::PlayerID idOfDeadPlayer = Netcode::getComponentOwner(networkIdOfKilled);
		std::string deadPlayer = NWrapperSingleton::getInstance().getPlayer(idOfDeadPlayer)->name;
		std::string ShooterPlayer = NWrapperSingleton::getInstance().getPlayer(playerIdOfShooter)->name;
		std::string deathType = "sprayed down";
		Logger::Log(ShooterPlayer + " " + deathType + " " + deadPlayer);

		m_gameDataTracker->logPlayerDeath(ShooterPlayer, deadPlayer, deathType);

		//This should remove the candle entity from game
		e->removeDeleteAllChildren();

		// Check if the extinguished candle is owned by the player
		if (Netcode::getComponentOwner(networkIdOfKilled) == m_playerID) {
			//If it is me that died, become spectator.
			e->addComponent<SpectatorComponent>();
			e->getComponent<MovementComponent>()->constantAcceleration = glm::vec3(0.f);
			e->getComponent<MovementComponent>()->velocity = glm::vec3(0.f);
			e->removeComponent<GunComponent>();

			e->getComponent<NetworkSenderComponent>()->removeAllMessageTypes();

			auto transform = e->getComponent<TransformComponent>();
			auto pos = glm::vec3(transform->getCurrentTransformState().m_translation);
			pos.y = 20.f;
			transform->setStartTranslation(pos);
			MapComponent temp;
			auto middleOfLevel = glm::vec3(temp.tileSize * temp.xsize / 2.f, 0.f, temp.tileSize * temp.ysize / 2.f);
			auto dir = glm::normalize(middleOfLevel - pos);
			auto rots = Utils::getRotations(dir);
			transform->setRotations(glm::vec3(0.f, -rots.y, rots.x));
		} else {
			//If it wasn't me that died, completely remove the player entity from game.
			e->queueDestruction();
		}
		return;
	}
	Logger::Warning("playerDied called but no matching entity found");
}

// NOTE: This is not called on the host, since the host receives the disconnect through NWrapperHost::playerDisconnected()
void NetworkReceiverSystem::playerDisconnect(Netcode::PlayerID playerID) {
	for (auto& e : entities) {
		if (Netcode::getComponentOwner(e->getComponent<NetworkReceiverComponent>()->m_id) == playerID) {

			e->removeDeleteAllChildren();
			// TODO: Remove all the components that can/should be removed

			e->queueDestruction();

			return;
		}
	}
	Logger::Warning("playerDisconnect called but no matching entity found");
}


// The player who puts down their candle does this in CandleSystem and tests collisions
// The candle will be moved for everyone else in here
void NetworkReceiverSystem::setCandleHeldState(Netcode::ComponentID id, bool isHeld, const glm::vec3& pos) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id != id) {
			continue;
		}

		for (int i = 0; i < e->getChildEntities().size(); i++) {
			if (auto candleE = e->getChildEntities()[i];  candleE->hasComponent<CandleComponent>()) {
				auto candleComp = candleE->getComponent<CandleComponent>();
				auto candleTransComp = candleE->getComponent<TransformComponent>();


				candleComp->setCarried(isHeld);
				candleComp->setWasCarriedLastUpdate(isHeld);
				if (!isHeld) {
					candleTransComp->removeParent();
					candleTransComp->setStartTranslation(pos);
					candleTransComp->setRotations(glm::vec3{ 0.f,0.f,0.f });
					e->getComponent<AnimationComponent>()->leftHandEntity = nullptr;


					// Might be needed
					ECS::Instance()->getSystem<UpdateBoundingBoxSystem>()->update(0.0f);
				} else {
					candleTransComp->setTranslation(glm::vec3(10.f, 2.0f, 0.f));
					candleTransComp->setParent(e->getComponent<TransformComponent>());

					e->getComponent<AnimationComponent>()->leftHandEntity = candleE.get();
				}
				return;
			}
		}
	}
	Logger::Warning("setCandleHeldState called but no matching entity found");
}

void NetworkReceiverSystem::shootStart(glm::vec3& gunPos, glm::vec3& gunVel, Netcode::ComponentID id) {
	// Spawn projectile
	projectileSpawned(gunPos, gunVel);

	// Find out who sent it and make them play the sound (locally)
	for (auto& e : entities) {
		// If we've found who sent the message
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_START].isPlaying = true;
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_START].playOnce = true;
		}
	}
}

void NetworkReceiverSystem::shootLoop(glm::vec3& gunPos, glm::vec3& gunVel, Netcode::ComponentID id) {
	// Spawn projectile
	projectileSpawned(gunPos, gunVel);

	// Find out who sent it and make them play the sound (locally)
	for (auto& e : entities) {
		// If we've found who sent the message
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {

			// Stop Start
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_START].isPlaying = false;

			// Play Loop
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_LOOP].isPlaying = true;
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_LOOP].playOnce = true;
		}
	}
}

void NetworkReceiverSystem::shootEnd(glm::vec3& gunPos, glm::vec3& gunVel, Netcode::ComponentID id) {
	// Spawn projectile
	projectileSpawned(gunPos, gunVel);

	// Find out who sent it and make them play the sound (locally)
	for (auto& e : entities) {
		// If we've found who sent the message
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			// Stop 
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_LOOP].isPlaying = false;

			// Start the end sound
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_END].isPlaying = true;
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_END].playOnce = true;
		}
	}
}


void NetworkReceiverSystem::matchEnded() {
	m_gameStatePtr->requestStackPop();
	m_gameStatePtr->requestStackPush(States::EndGame);
}

void NetworkReceiverSystem::backToLobby() {
	m_gameStatePtr->requestStackPop();
	m_gameStatePtr->requestStackPush(States::JoinLobby);
}
