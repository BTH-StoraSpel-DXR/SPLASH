#include "pch.h"
#include "NetworkReceiverSystem.h"
#include "Sail/entities/Entity.h"
#include "Sail/entities/components/NetworkReceiverComponent.h"
#include "Sail/entities/systems/network/NetworkSenderSystem.h"
#include "Sail/entities/components/OnlineOwnerComponent.h"
#include "Sail/entities/components/LocalOwnerComponent.h"
#include "../SPLASH/src/game/states/GameState.h"

#include "Network/NWrapperSingleton.h"
#include "Sail/../../libraries/cereal/archives/portable_binary.hpp"

// Creation of mid-air bullets from here.
#include "Sail/entities/systems/Gameplay/GunSystem.h"


#define BANNED(func) sorry_##func##_is_a_banned_function

// The host will now automatically forward all incoming messages to other players so
// no need to use any host-specific logic in this system
#undef isHost
#define isHost() BANNED(isHost())


NetworkReceiverSystem::NetworkReceiverSystem() : BaseComponentSystem() {
	registerComponent<NetworkReceiverComponent>(true, true, true);
	registerComponent<TransformComponent>(false, true, true);
}

NetworkReceiverSystem::~NetworkReceiverSystem() {
}

void NetworkReceiverSystem::init(unsigned char playerID, GameState* gameStatePtr, NetworkSenderSystem* netSendSysPtr) {
	m_playerID = playerID;
	m_gameStatePtr = gameStatePtr;
	m_netSendSysPtr = netSendSysPtr;
}

void NetworkReceiverSystem::initPlayer(Entity* pPlayerEntity) {
	m_playerEntity = pPlayerEntity;
}

const std::vector<Entity*>& NetworkReceiverSystem::getEntities() const {
	return entities;
}

/*
  The parsing of messages needs to match how the NetworkSenderSystem constructs them so
  any changes made here needs to be made there as well!

  Logical structure of the packages that will be decoded by this function:

	__int32         nrOfEntities
	__int32         senderID

		NetworkObjectID entity[0].id
		EntityType		entity[0].type
		__int32			nrOfMessages
			MessageType     entity[0].messageType
			MessageData     entity[0].data

		NetworkObjectID entity[0].id
		EntityType		entity[0].type
		__int32			nrOfMessages
			MessageType     entity[0].messageType
			MessageData     entity[0].data

		NetworkObjectID entity[0].id
		EntityType		entity[0].type
		__int32			nrOfMessages
			MessageType     entity[0].messageType
			MessageData     entity[0].data
		....

*/
void NetworkReceiverSystem::update() {
	using namespace Netcode;

	// Don't push more data to the buffer whilst this function is running
	std::scoped_lock lock(m_bufferLock);

	__int32 nrOfObjectsInMessage = 0;
	unsigned char senderID = 0;
	NetworkObjectID id = 0;
	Netcode::MessageType messageType;
	Netcode::EntityType entityType;
	__int32 messageTypesSize = 0;
	glm::vec3 translation;
	glm::vec3 rotation;
	glm::vec3 gunPosition;
	glm::vec3 gunVelocity;

	// Process all messages in the buffer
	while (!m_incomingDataBuffer.empty()) {
		std::istringstream is(m_incomingDataBuffer.front());
		cereal::PortableBinaryInputArchive ar(is);
		{
			// Read message metadata
			ar(nrOfObjectsInMessage);

			// Get the ID of whoever sent the package so that we can ignore it if it was sent by ourself.
			ar(senderID);
		}
		// If the packet was sent from ourself then don't process it
		// NOTE: Because of this we no longer need to check if parts of messages were sent from ourselves
		if (senderID == m_playerID) {
			m_incomingDataBuffer.pop();
			continue; // This will go to the next iteration of the while loop.
		}

		// Read and process data
		for (int i = 0; i < nrOfObjectsInMessage; ++i) {
			ar(id);				// NetworkObject-ID
			ar(entityType);		//
			ar(messageTypesSize);	//

			// Read per data type
			for (int j = 0; j < messageTypesSize; j++) {
				ar(messageType);

				// Read and process the data
				switch (messageType) {
					// Send necessary info to create the networked entity 
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
				/* Case Animation Data, int, float */
				break;
				default:
					break;
				}
			}
		}


		// Receive 'one-time' events
		__int32 eventSize;
		Netcode::MessageType eventType;
		__int32 netObjectID;
		ar(eventSize);


		for (int i = 0; i < eventSize; i++) {
			// Handle-Single-Frame events
			ar(eventType);

			// NEW STUFF

			/* READ ALL DATA */

			/* EARLY EXIT */

			/* */

			if (eventType == Netcode::MessageType::PLAYER_JUMPED) {
				ar(netObjectID);
				playerJumped(netObjectID);
			}
			else if (eventType == Netcode::MessageType::WATER_HIT_PLAYER) {
				ar(netObjectID);
				waterHitPlayer(netObjectID);
			}
			else if (eventType == Netcode::MessageType::SPAWN_PROJECTILE) {
				ArchiveHelpers::loadVec3(ar, gunPosition);
				ArchiveHelpers::loadVec3(ar, gunVelocity);

				EntityFactory::CreateProjectile(gunPosition, gunVelocity, false, 100, 4, 0); //Owner id not set, 100 for now.
			}

			else if (eventType == Netcode::MessageType::PLAYER_DIED) {
				ar(netObjectID);
				playerDied(netObjectID);
			}
			else if (eventType == Netcode::MessageType::MATCH_ENDED) {
				matchEnded();
			}
			else if (eventType == Netcode::MessageType::CANDLE_HELD_STATE) {
				glm::vec3 candlepos;
				bool isCarried;
				ar(netObjectID);

				ar(isCarried);
				ArchiveHelpers::loadVec3(ar, candlepos);
				setCandleHeldState(netObjectID, isCarried, candlepos);
			}
			else if (eventType == Netcode::MessageType::SEND_ALL_BACK_TO_LOBBY) {
				backToLobby();
			}
			else if (eventType == Netcode::MessageType::PLAYER_DISCONNECT) {
				unsigned char playerID;

				ar(playerID);
				playerDisconnect(playerID);
			}
		}


		m_incomingDataBuffer.pop();
	}
}

/*
  Creates a new entity of the specified entity type and with a NetworkReceiverComponent attached to it

  TODO: Use an entity factory with blueprints or something like that instead of manually constructing entities here
*/
void NetworkReceiverSystem::createEntity(Netcode::NetworkObjectID id, Netcode::EntityType entityType, const glm::vec3& translation) {
	using namespace Netcode;

	// Early exit if the entity already exists
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			return;
		}
	}

	auto e = ECS::Instance()->createEntity("ReceiverEntity");
	entities.push_back(e.get());	// Needs to be before 'addComponent' or packets might be lost.
	e->addComponent<NetworkReceiverComponent>(id, entityType);
	int test = e->getComponent<NetworkReceiverComponent>()->m_id;
	e->addComponent<OnlineOwnerComponent>(id);

	std::string modelName = "DocGunRun4.fbx";
	auto* shader = &Application::getInstance()->getResourceManager().getShaderSet<GBufferOutShader>();
	Model* characterModel = &Application::getInstance()->getResourceManager().getModelCopy(modelName, shader);
	characterModel->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Character/CharacterMRAO.tga");
	characterModel->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Character/CharacterTex.tga");
	characterModel->getMesh(0)->getMaterial()->setNormalTexture("pbr/Character/CharacterNM.tga");
	characterModel->setIsAnimated(true);
	AnimationStack* stack = &Application::getInstance()->getResourceManager().getAnimationStack(modelName);

	auto* wireframeShader = &Application::getInstance()->getResourceManager().getShaderSet<WireframeShader>();
	Model* lightModel = &Application::getInstance()->getResourceManager().getModel("candleExported.fbx", shader);
	lightModel->getMesh(0)->getMaterial()->setAlbedoTexture("sponza/textures/candleBasicTexture.tga");
	//Wireframe bounding box model
	Model* boundingBoxModel = &Application::getInstance()->getResourceManager().getModel("boundingBox.fbx", wireframeShader);
	boundingBoxModel->getMesh(0)->getMaterial()->setColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	// create the new entity
	switch (entityType) {
	case EntityType::PLAYER_ENTITY:
	{
		e->addComponent<ModelComponent>(characterModel);
		AnimationComponent* ac = e->addComponent<AnimationComponent>(stack);
		ac->currentAnimation = stack->getAnimation(1);
		e->addComponent<TransformComponent>(translation);
		e->addComponent<BoundingBoxComponent>(boundingBoxModel);
		e->addComponent<CollidableComponent>();

		// Adding audio component and adding all sounds attached to the player entity
		e->addComponent<AudioComponent>();

		// RUN Sound
		Audio::SoundInfo sound{};
		sound.fileName = "../Audio/footsteps_1.wav";
		sound.soundEffectLength = 1.0f;
		sound.volume = 0.5f;
		sound.playOnce = false;
		e->getComponent<AudioComponent>()->defineSound(Audio::SoundType::RUN, sound);
		// JUMP Sound
		sound.fileName = "../Audio/jump.wav";
		sound.soundEffectLength = 0.7f;
		sound.playOnce = true;
		e->getComponent<AudioComponent>()->defineSound(Audio::SoundType::JUMP, sound);

		//creates light with model and pointlight
		auto light = ECS::Instance()->createEntity("ReceiverLight");
		light->addComponent<CandleComponent>();
		light->addComponent<ModelComponent>(lightModel);
		light->addComponent<TransformComponent>(glm::vec3(0.f, 2.f, 0.f));
		light->addComponent<BoundingBoxComponent>(boundingBoxModel);
		light->addComponent<CollidableComponent>();
		light->addComponent<OnlineOwnerComponent>(id);
		PointLight pl;
		pl.setColor(glm::vec3(0.2f, 0.2f, 0.2f));
		pl.setPosition(glm::vec3(0.2f, 0.2f + .37f, 0.2f));
		pl.setAttenuation(.0f, 0.1f, 0.02f);
		//pl.setIndex(m_currLightIndex++);
		pl.setIndex(999); // TODO: unique light index needed?
		light->addComponent<LightComponent>(pl);

		e->addChildEntity(light);
	}
	break;
	default:
		break;
	}
}

// Might need some optimization (like sorting) if we have a lot of networked entities
void NetworkReceiverSystem::setEntityTranslation(Netcode::NetworkObjectID id, const glm::vec3& translation) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			glm::vec3 pos = e->getComponent<TransformComponent>()->getTranslation();
			if (pos != translation) {
				if (e->getComponent<AnimationComponent>()->currentAnimation == e->getComponent<AnimationComponent>()->getAnimationStack()->getAnimation(0) && e->getComponent<AnimationComponent>()->transitions.size() == 0) {
					e->getComponent<AnimationComponent>()->transitions.emplace(e->getComponent<AnimationComponent>()->getAnimationStack()->getAnimation(1), 0.01f, false);
				}
			}
			else {
				if (e->getComponent<AnimationComponent>()->currentAnimation == e->getComponent<AnimationComponent>()->getAnimationStack()->getAnimation(1) && e->getComponent<AnimationComponent>()->transitions.size() == 0) {
					e->getComponent<AnimationComponent>()->transitions.emplace(e->getComponent<AnimationComponent>()->getAnimationStack()->getAnimation(0), 0.01f, false);
				}
			}
			e->getComponent<TransformComponent>()->setTranslation(translation);

			break;
		}
	}
}

void NetworkReceiverSystem::setEntityRotation(Netcode::NetworkObjectID id, const glm::vec3& rotation) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			//TODO: REMOVE THIS WHEN NEW ANIMATIONS ARE PUT IN
			//TODO: REMOVE
			//TODO: REMOVE	//TODO: REMOVE THIS WHEN NEW ANIMATIONS ARE PUT IN
			glm::vec3 rot = rotation;
			if (e->getComponent<AnimationComponent>()->currentAnimation != e->getComponent<AnimationComponent>()->getAnimationStack()->getAnimation(0)) {
				rot.y += 3.14f * 0.5f;
			}
			e->getComponent<TransformComponent>()->setRotations(rot);
			break;
		}
	}
}

void NetworkReceiverSystem::playerJumped(Netcode::NetworkObjectID id) {
	// How do i trigger a jump from here?
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::JUMP].playOnce = true;
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::JUMP].isPlaying = true;

			break;
		}
	}
}

void NetworkReceiverSystem::waterHitPlayer(Netcode::NetworkObjectID id) {
	for (auto& e : entities) {
		//Look for the entity that OWNS the candle (player entity)
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			//Look for the entity that IS the candle (candle entity)
			std::vector<Entity::SPtr> childEntities = e->getChildEntities();
			for (auto& child : childEntities) {
				if (child->hasComponent<CandleComponent>()) {
					// Damage the candle
					child->getComponent<CandleComponent>()->hitWithWater(10.0f);
					// Check in Candle System What happens next
					break;
				}

			}

			break;
		}
	}
}

void NetworkReceiverSystem::playerDied(Netcode::NetworkObjectID id) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {

			//This should remove the candle entity from game
			e->removeDeleteAllChildren();

			// Check if the extinguished candle is owned by the player
			if (id >> 18 == m_playerID) {
				//If it is me that died, become spectator.
				e->addComponent<SpectatorComponent>();
				e->getComponent<MovementComponent>()->constantAcceleration = glm::vec3(0.f, 0.f, 0.f);
				e->removeComponent<GunComponent>();
			}
			else {
				//If it wasnt me that died, compleatly remove the player entity from game.
				e->queueDestruction();
			}

			break; // Break because should only be one player; stop looping!
		}
	}
}

void NetworkReceiverSystem::playerDisconnect(unsigned char id) {
	// This is not called on the host, since the host receives the disconnect through NWrapperHost::playerDisconnected()	
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id >> 18 == id) {

			e->removeDeleteAllChildren();
			// TODO: Remove all the components that can/should be removed

			e->queueDestruction();

			break; // Break because should only be one candle; stop looping!
		}
	}
}
void NetworkReceiverSystem::setCandleHeldState(Netcode::NetworkObjectID id, bool b, const glm::vec3& pos) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {

			for (int i = 0; i < e->getChildEntities().size(); i++) {
				auto candleE = e->getChildEntities()[i];

				if (candleE->hasComponent<CandleComponent>()) {
					auto candleComp = candleE->getComponent<CandleComponent>();

					candleComp->setCarried(b);
					if (!b) {
						candleE->getComponent<TransformComponent>()->setTranslation(pos);
					}
					return;
				}
			}

			break;
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
