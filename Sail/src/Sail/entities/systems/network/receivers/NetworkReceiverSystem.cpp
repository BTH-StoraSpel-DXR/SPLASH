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


// Creation of mid-air bullets from here.
#include "Sail/entities/systems/Gameplay/GunSystem.h"


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
  ---------------------------------------------------
	PlayerID        senderID
	size_t          nrOfEntities
	    ComponentID     entity[0].id
	    EntityType      entity[0].type
	    size_t          nrOfMessages
	        MessageType     entity[0].messageType
	        MessageData     entity[0].data
	        ...
	    ComponentID     entity[1].id
	    EntityType      entity[1].type
	    size_t          nrOfMessages
	        MessageType     entity[0].messageType
	        MessageData     entity[0].data
	        ...
	    ComponentID     entity[2].id
	    EntityType      entity[2].type
	    size_t          nrOfMessages
	        MessageType     entity[0].messageType
	        MessageData     entity[0].data
	        ...
	    ...
	size_t          nrOfEvents
	    MessageType     eventType[0]
	    EventData       eventData[0]
	    ...
	...
  ---------------------------------------------------

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
			ar(id);               // NetworkObject-ID
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
				/* Case Animation Data, int, float */
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
				ArchiveHelpers::loadVec3(ar, gunPosition);
				ArchiveHelpers::loadVec3(ar, gunVelocity);


				projectileSpawned(gunPosition, gunVelocity);
			}
			break;
			case Netcode::MessageType::PLAYER_DIED:
			{
				Netcode::PlayerID playerIdOfShooter;
				Netcode::ComponentID networkIdOfKilled;

				ar(networkIdOfKilled); // Recieve
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
				unsigned char playerID;

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

  TODO: Use an entity factory with blueprints or something like that instead of manually constructing entities here
*/
void NetworkReceiverSystem::createEntity(Netcode::ComponentID id, Netcode::EntityType entityType, const glm::vec3& translation) {
	// Early exit if the entity already exists
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			return;
		}
	}

	// -------------------------------------------
	// TODO: THIS SECTION SHOULD BE A PART OF ENTITY FACTORY
	std::string modelName = "DocTorch.fbx";
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
	// -------------------------------------------


	auto e = ECS::Instance()->createEntity("ReceiverEntity");

	// Manually add the entity to this system in case there's another message telling us to modify it, don't wait for ECS
	entities.push_back(e.get());	// Needs to happen before any 'addComponent' or packets might be lost.


	e->addComponent<NetworkReceiverComponent>(id, entityType);
	e->addComponent<OnlineOwnerComponent>(id);

	// create the new entity
	switch (entityType) {
	case Netcode::EntityType::PLAYER_ENTITY:
	{
		e->addComponent<ModelComponent>(characterModel);
		AnimationComponent* ac = e->addComponent<AnimationComponent>(stack);
		ac->currentAnimation = stack->getAnimation(3);
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
		// SHOOT sound
		sound.fileName = "../Audio/testSoundShoot.wav";
		sound.soundEffectLength = 1.0f;
		sound.playOnce = true;

		//creates light with model and pointlight
		auto light = ECS::Instance()->createEntity("ReceiverLight");
		light->addComponent<CandleComponent>();
		light->addComponent<ModelComponent>(lightModel);
		light->addComponent<TransformComponent>();
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
		ac->leftHandEntity = light.get();
		
		// PUT THINGS IN HAND
		ac->leftHandPosition = glm::identity<glm::mat4>();
		ac->leftHandPosition = glm::translate(ac->leftHandPosition, glm::vec3(0.57f, 1.03f, 0.05f));
		ac->leftHandPosition = ac->leftHandPosition * glm::toMat4(glm::quat(glm::vec3(3.14f * 0.5f, -3.14f * 0.17f, 0.0f)));






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
			break;
		}
	}
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

			break;
		}
	}
}

void NetworkReceiverSystem::setEntityAnimation(Netcode::ComponentID id, int animationStack, float animationTime) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			auto animation = e->getComponent<AnimationComponent>();
			animation->currentAnimation = animation->getAnimationStack()->getAnimation(animationStack);
			animation->animationTime = animationTime;
		}
	}
}

void NetworkReceiverSystem::playerJumped(Netcode::ComponentID id) {
	// How do i trigger a jump from here?
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::JUMP].playOnce = true;
			e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::JUMP].isPlaying = true;

			break;
		}
	}
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
				break;
			}
		}
		break;
	}
}

void NetworkReceiverSystem::projectileSpawned(glm::vec3& pos, glm::vec3 dir) {
	// Also play the sound


	EntityFactory::CreateProjectile(pos, dir, false, 100, 4, 0); //Owner id not set, 100 for now.
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
		Logger::Log(ShooterPlayer + " sprayed down " + deadPlayer);

		//This should remove the candle entity from game
		e->removeDeleteAllChildren();

		// Check if the extinguished candle is owned by the player
		if (Netcode::getComponentOwner(networkIdOfKilled) == m_playerID) {
			//If it is me that died, become spectator.
			e->addComponent<SpectatorComponent>();
			e->getComponent<MovementComponent>()->constantAcceleration = glm::vec3(0.f);
			e->getComponent<MovementComponent>()->velocity = glm::vec3(0.f);
			e->removeComponent<GunComponent>();

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
		break; // Break because should only be one player; stop looping!
	}
}

// NOTE: This is not called on the host, since the host receives the disconnect through NWrapperHost::playerDisconnected()
void NetworkReceiverSystem::playerDisconnect(Netcode::PlayerID playerID) {
	for (auto& e : entities) {
		if (Netcode::getComponentOwner(e->getComponent<NetworkReceiverComponent>()->m_id) == playerID) {

			e->removeDeleteAllChildren();
			// TODO: Remove all the components that can/should be removed

			e->queueDestruction();

			break; // Break because should only be one candle; stop looping!
		}
	}
}

void NetworkReceiverSystem::setCandleHeldState(Netcode::ComponentID id, bool b, const glm::vec3& pos) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id != id) {
			continue;
		}

		for (int i = 0; i < e->getChildEntities().size(); i++) {
			if (auto candleE = e->getChildEntities()[i];  candleE->hasComponent<CandleComponent>()) {
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


void NetworkReceiverSystem::matchEnded() {
	m_gameStatePtr->requestStackPop();
	m_gameStatePtr->requestStackPush(States::EndGame);
}

void NetworkReceiverSystem::backToLobby() {
	m_gameStatePtr->requestStackPop();
	m_gameStatePtr->requestStackPush(States::JoinLobby);
}
