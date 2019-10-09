#include "pch.h"
#include "NetworkReceiverSystem.h"
#include "Sail/entities/Entity.h"
#include "Sail/entities/components/NetworkReceiverComponent.h"

#include "Network/NWrapperSingleton.h"
#include "Sail/../../libraries/cereal/archives/portable_binary.hpp"

// Creation of mid-air bullets from here.
#include "Sail/entities/systems/Gameplay/GunSystem.h"

NetworkReceiverSystem::NetworkReceiverSystem() : BaseComponentSystem() {
	registerComponent<NetworkReceiverComponent>(true, true, true);
	
	// Can create entities with these components:
	registerComponent<NetworkSenderComponent>(false, true, true);
	registerComponent<ModelComponent>(false, true, true);
	registerComponent<TransformComponent>(false, true, true);
	registerComponent<BoundingBoxComponent>(false, true, true);
	registerComponent<CollidableComponent>(false, true, true);
}

NetworkReceiverSystem::~NetworkReceiverSystem() 
{}

void NetworkReceiverSystem::initWithPlayerID(unsigned char playerID) {
	m_playerID = playerID;
}

// Push incoming data strings to the back of a FIFO list
void NetworkReceiverSystem::pushDataToBuffer(std::string data) {
	std::scoped_lock lock(m_bufferLock);
	m_incomingDataBuffer.push(data);
}

/*
  The parsing of messages needs to match how the NetworkSenderSystem constructs them so
  any changes made here needs to be made there as well!

  Logical structure of the packages that will be decoded by this function:

	__int32         nrOfEntities

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
void NetworkReceiverSystem::update(float dt) {
	using namespace Netcode;

	// Don't push more data to the buffer whilst this function is running
	std::scoped_lock lock(m_bufferLock);

	NetworkObjectID id = 0;
	Netcode::MessageType dataType;
	__int32 messageTypes = 0;
	EntityType entityType;
	glm::vec3 translation;
	glm::vec3 rotation;
	glm::vec3 gunPosition;
	glm::vec3 gunDirection;

	// Process all messages in the buffer
	while (!m_incomingDataBuffer.empty()) {
		std::istringstream is(m_incomingDataBuffer.front());
		cereal::PortableBinaryInputArchive ar(is);

		// Read entityCount
		__int32 nrOfEntitiesInMessage = 0;
		{
			ar(nrOfEntitiesInMessage);
		}


		// Read the data per entity
		for (int i = 0; i < nrOfEntitiesInMessage; ++i) {
			{
				ar(id);					// Entity.id
				ar(entityType);			// Entity type
				ar(messageTypes);		// Nr of Messages (for this entity)
			}

			// Read per data type
			for (int j = 0; j < messageTypes; j++) {
				ar(dataType);

				// Read and process the data
				switch (dataType) {
					// Send necessary info to create the networked entity 
				case MessageType::CREATE_NETWORKED_ENTITY:
				{
					Archive::loadVec3(ar, translation); // Read translation
					createEntity(id, entityType, translation);
				}
				break;
				case MessageType::MODIFY_TRANSFORM:
				{
					Archive::loadVec3(ar, translation); // Read translation
					setEntityTranslation(id, translation);
				}
				break;
				case MessageType::ROTATION_TRANSFORM:
				{
					Archive::loadVec3(ar, rotation);	// Read rotation
					setEntityRotation(id, rotation);
				}
				break;
				case MessageType::SPAWN_PROJECTILE:
				{
					// Read data
					Archive::loadVec3(ar, gunPosition);
					Archive::loadVec3(ar, gunDirection);

					// Use data
					GunFactory::createWaterBullet(gunPosition, gunDirection, 0);
					
				}
				break;
				default:
					break;
				}
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

	// Early exit if the entity belongs to the player 
	// (since host sends out messages to all players including the one who sent it to them)
	if (static_cast<unsigned char>(id >> 18) == m_playerID) { // First byte is always the ID of the player who created the object
		return;
	}

	// Early exit if the entity already exists
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			return;
		}
	}
	
	auto e = ECS::Instance()->createEntity("ReceiverEntity");
	e->addComponent<NetworkReceiverComponent>(id, entityType);

	// If you are the host create a pass-through sender component to pass on the info to all players
	if (NWrapperSingleton::getInstance().isHost()) {
		// NOTE: Assumes that the data type is MODIFY_TRANSFORM, might be changed in the future
		e->addComponent<NetworkSenderComponent>(MessageType::CREATE_NETWORKED_ENTITY, entityType, id);
	}

	auto* shader = &Application::getInstance()->getResourceManager().getShaderSet<GBufferOutShader>();
	Model* characterModel = &Application::getInstance()->getResourceManager().getModel("character1.fbx", shader);
	characterModel->getMesh(0)->getMaterial()->setAlbedoTexture("sponza/textures/character1texture.tga");
	auto* wireframeShader = &Application::getInstance()->getResourceManager().getShaderSet<WireframeShader>();
	Model* lightModel = &Application::getInstance()->getResourceManager().getModel("candleExported.fbx", shader);
	lightModel->getMesh(0)->getMaterial()->setAlbedoTexture("sponza/textures/candleBasicTexture.tga");
	//Wireframe bounding box model
	Model* boundingBoxModel = &Application::getInstance()->getResourceManager().getModel("boundingBox.fbx", wireframeShader);
	boundingBoxModel->getMesh(0)->getMaterial()->setColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

	// create the new entity
	switch (entityType) {
	case EntityType::PLAYER_ENTITY:
	{
		e->addComponent<ModelComponent>(characterModel);
		e->addComponent<TransformComponent>(translation);
		e->addComponent<BoundingBoxComponent>(boundingBoxModel);
		e->addComponent<CollidableComponent>();

		//creates light with model and pointlight
		auto light = ECS::Instance()->createEntity("ReceiverLight");
		light->addComponent<CandleComponent>();
		light->addComponent<ModelComponent>(lightModel);
		light->addComponent<TransformComponent>(glm::vec3(0.f, 2.f, 0.f));
		light->addComponent<BoundingBoxComponent>(boundingBoxModel);
		light->addComponent<CollidableComponent>();
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

	// Manually add the entity to this system in case there's another message telling us to modify it, don't wait for ECS
	entities.push_back(e.get());
}

// Might need some optimization (like sorting) if we have a lot of networked entities
void NetworkReceiverSystem::setEntityTranslation(Netcode::NetworkObjectID id, const glm::vec3& translation) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			e->getComponent<TransformComponent>()->setTranslation(translation);
			break;
		}
	}
}

void NetworkReceiverSystem::setEntityRotation(Netcode::NetworkObjectID id, const glm::vec3& rotation){
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			e->getComponent<TransformComponent>()->setRotations(rotation);
			break;
		}
	}

}

