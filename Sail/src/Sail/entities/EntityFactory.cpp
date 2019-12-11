#include "pch.h"
#include "EntityFactory.hpp"

#include "Sail/Application.h"
#include "Sail/graphics/geometry/Model.h"
#include "Sail/entities/components/Components.h"
#include "Sail/ai/pathfinding/NodeSystem.h"
#include "Sail/ai/states/AttackingState.h"
#include "Sail/ai/states/FleeingState.h"
#include "Sail/ai/states/SearchingState.h"
#include "../Sail/src/Network/NWrapperSingleton.h"
#include "Sail/graphics/geometry/factory/StringModel.h"
#include "Sail/graphics/geometry/factory/QuadModel.h"

#include "Sail/entities/systems/network/receivers/KillCamReceiverSystem.h"


//#define LOG_NET_COMP_ID

void EntityFactory::CreateCandle(Entity::SPtr& candle, const glm::vec3& lightPos, size_t lightIndex) {
	// Candle has a model and a bounding box
	auto* shader = &Application::getInstance()->getResourceManager().getShaderSet<GBufferOutShader>();
	Model* candleModel = &Application::getInstance()->getResourceManager().getModel("Torch.fbx", shader);
	candleModel->setCastShadows(false);
	candleModel->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Torch/Torch_Albedo.dds");
	candleModel->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Torch/Torch_NM.dds");
	candleModel->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Torch/Torch_MRAO.dds");


	auto* wireframeShader = &Application::getInstance()->getResourceManager().getShaderSet<GBufferWireframe>();
	Model* boundingBoxModel = &Application::getInstance()->getResourceManager().getModel("boundingBox.fbx", wireframeShader);
	boundingBoxModel->getMesh(0)->getMaterial()->setColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	boundingBoxModel->getMesh(0)->getMaterial()->setAOScale(0.5);
	boundingBoxModel->getMesh(0)->getMaterial()->setMetalnessScale(0.5);
	boundingBoxModel->getMesh(0)->getMaterial()->setRoughnessScale(0.5);


	//creates light with model and pointlight
	candle->addComponent<ModelComponent>(candleModel);
	candle->addComponent<TransformComponent>(lightPos);
	candle->addComponent<BoundingBoxComponent>(boundingBoxModel);
	candle->addComponent<CullingComponent>();

	auto* particleEmitterComp = candle->addComponent<ParticleEmitterComponent>();

	particleEmitterComp->size = 0.1f;
	particleEmitterComp->offset = { 0.0f, 0.44f, 0.0f };
	particleEmitterComp->constantVelocity = { 0.0f, 0.2f, 0.0f };
	particleEmitterComp->acceleration = { 0.0f, 1.0f, 0.0f };
	particleEmitterComp->spread = { 0.1f, 0.1f, 0.1f };
	particleEmitterComp->spawnRate = 0.01f;
	particleEmitterComp->lifeTime = 0.13f;
	particleEmitterComp->maxNumberOfParticles = 100;
	std::string particleTextureName = "particles/animFire.dds";
	particleEmitterComp->atlasSize = glm::uvec2(8U, 4U);
	if (!Application::getInstance()->getResourceManager().hasTexture(particleTextureName)) {
		Application::getInstance()->getResourceManager().loadTexture(particleTextureName);
	}
	particleEmitterComp->setTexture(particleTextureName);

	auto* ragdollComp = candle->addComponent<RagdollComponent>(boundingBoxModel);
	ragdollComp->localCenterOfMass = { 0.f, 0.2f, 0.f };
	ragdollComp->addContactPoint(glm::vec3(0.f), glm::vec3(0.08f));
	ragdollComp->addContactPoint(glm::vec3(0.f, 0.37f, 0.0f), glm::vec3(0.08f));

	PointLight pl;
	pl.setColor(glm::vec3(0.55f, 0.5f, 0.45f));
	pl.setPosition(glm::vec3(lightPos.x, lightPos.y + .5f, lightPos.z));
	pl.setRadius(10.f);
	pl.setIndex(lightIndex);
	candle->addComponent<LightComponent>(pl);
}

Entity::SPtr EntityFactory::CreateWaterGun(const std::string& name) {

	auto* shader = &Application::getInstance()->getResourceManager().getShaderSet<GBufferOutShader>();
	Model* candleModel = &Application::getInstance()->getResourceManager().getModel("WaterPistol.fbx", shader);
	candleModel->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/WaterGun/Watergun_Albedo.dds");
	candleModel->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/WaterGun/Watergun_MRAO.dds");
	candleModel->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/WaterGun/Watergun_NM.dds");


	auto gun = ECS::Instance()->createEntity(name.c_str());
	gun->addComponent<ModelComponent>(candleModel);
	gun->addComponent<TransformComponent>();
	gun->addComponent<CullingComponent>();

	return gun;
}


void EntityFactory::AddCandleComponentsToPlayer(Entity::SPtr& player, const size_t& lightIndex, const Netcode::PlayerID& playerID) {
	for (Entity* c : player->getChildEntities()) {
		if (c->getName() == player->getName() + "Candle") {
			c->addComponent<CandleComponent>()->playerEntityID = playerID;
			c->addComponent<CollidableComponent>();
			c->addComponent<TeamComponent>()->team = NWrapperSingleton::getInstance().getPlayer(playerID)->team;
		}
	}
}

Entity::SPtr EntityFactory::CreateMyPlayer(Netcode::PlayerID playerID, size_t lightIndex, glm::vec3 spawnLocation) {
	const Netcode::ComponentID playerCompID = Netcode::getPlayerCompID(playerID);
	const Netcode::ComponentID torchCompID = Netcode::getTorchCompID(playerID);
	const Netcode::ComponentID gunCompID = Netcode::getGunCompID(playerID);

#ifdef LOG_NET_COMP_ID
	std::string logString = "My Player: " + std::to_string(playerID) + "\t" + std::to_string(playerCompID) + "\t" + std::to_string(gunCompID) + "\t" + std::to_string(torchCompID);
	SAIL_LOG(logString);
	OutputDebugStringA(logString.c_str());
#endif

	// Create my player
	auto myPlayer = ECS::Instance()->createEntity("MyPlayer");
	EntityFactory::CreateGenericPlayer(myPlayer, lightIndex, spawnLocation, playerID);

	auto senderC = myPlayer->addComponent<NetworkSenderComponent>(Netcode::EntityType::PLAYER_ENTITY, playerCompID);
	senderC->addMessageType(Netcode::MessageType::ANIMATION);
	senderC->addMessageType(Netcode::MessageType::CHANGE_LOCAL_POSITION);
	senderC->addMessageType(Netcode::MessageType::CHANGE_LOCAL_ROTATION);

	myPlayer->addComponent<NetworkReceiverComponent>(playerCompID, Netcode::EntityType::PLAYER_ENTITY);
	myPlayer->addComponent<LocalOwnerComponent>(playerCompID);
	myPlayer->addComponent<CollisionComponent>();
	myPlayer->getComponent<ModelComponent>()->renderToGBuffer = true;
	myPlayer->addComponent<MovementComponent>()->constantAcceleration = glm::vec3(0.0f, -9.8f, 0.0f);
	myPlayer->addComponent<RealTimeComponent>();
	myPlayer->addComponent<ThrowingComponent>();
	myPlayer->addComponent<RenderInActiveGameComponent>();



	AnimationComponent* ac = myPlayer->getComponent<AnimationComponent>();

	// Define the position for the camera
	ac->is_camFollowingHead = true;
	ac->headPositionMatrix = glm::translate(glm::identity<glm::mat4>(), ac->headPositionLocalDefault);

	AddCandleComponentsToPlayer(myPlayer, lightIndex, playerID);

	for (Entity* c : myPlayer->getChildEntities()) {
		if (c->getName() == myPlayer->getName() + "WaterGun") {
			c->addComponent<NetworkSenderComponent>(Netcode::EntityType::GUN_ENTITY, gunCompID);
			c->addComponent<NetworkReceiverComponent>(gunCompID, Netcode::EntityType::GUN_ENTITY);
			//leave this for now
			//c->addComponent<GunComponent>();]
			c->addComponent<RealTimeComponent>(); // The player's gun is updated each frame
			c->addComponent<RenderInActiveGameComponent>();
		}

		// Add a localOwnerComponent to our candle so that we can differentiate it from other candles
		if (c->hasComponent<CandleComponent>()) {
			c->addComponent<NetworkSenderComponent>(Netcode::EntityType::CANDLE_ENTITY, torchCompID);
			c->addComponent<NetworkReceiverComponent>(torchCompID, Netcode::EntityType::CANDLE_ENTITY);
			c->addComponent<LocalOwnerComponent>(playerCompID);
			c->addComponent<RealTimeComponent>(); // The player's candle is updated each frame
			c->addComponent<MovementComponent>();
			c->addComponent<RenderInActiveGameComponent>();
		}
	}

	// REPLAY COPY OF THE ENTITY
	CreateReplayPlayer(playerCompID, torchCompID, gunCompID, lightIndex, spawnLocation);

	return myPlayer;
}

// otherPlayer is an entity that doesn't have any components added to it yet.
// Needed so that NetworkReceiverSystem can add the entity to itself before 
void EntityFactory::CreateOtherPlayer(Entity::SPtr& otherPlayer, Netcode::PlayerID playerID, size_t lightIndex) {
	const Netcode::ComponentID playerCompID = Netcode::getPlayerCompID(playerID);
	const Netcode::ComponentID torchCompID = Netcode::getTorchCompID(playerID);
	const Netcode::ComponentID gunCompID = Netcode::getGunCompID(playerID);
	const glm::vec3 spawnLocation = { playerID * 10.f, -10.f, 0.f };

#ifdef LOG_NET_COMP_ID
	std::string logString = std::to_string(playerID) + "\t" + std::to_string(playerCompID) + "\t" + std::to_string(gunCompID) + "\t" + std::to_string(torchCompID);
	SAIL_LOG(logString);
	OutputDebugStringA(logString.c_str());
#endif

	EntityFactory::CreateGenericPlayer(otherPlayer, lightIndex, spawnLocation, Netcode::getComponentOwner(playerCompID));
	// Other players have a character model and animations

	auto rec = otherPlayer->addComponent<NetworkReceiverComponent>(playerCompID, Netcode::EntityType::PLAYER_ENTITY);

	otherPlayer->addComponent<OnlineOwnerComponent>(playerCompID);
	otherPlayer->addComponent<RenderInActiveGameComponent>();


	// Create the player
	AddCandleComponentsToPlayer(otherPlayer, lightIndex, Netcode::getComponentOwner(playerCompID));

	for (Entity* c : otherPlayer->getChildEntities()) {
		if (c->getName() == otherPlayer->getName() + "WaterGun") {

			auto rec = c->addComponent<NetworkReceiverComponent>(gunCompID, Netcode::EntityType::GUN_ENTITY);
			if (NWrapperSingleton::getInstance().isHost()) {
				c->addComponent<NetworkSenderComponent>(Netcode::EntityType::GUN_ENTITY, gunCompID)->m_id = rec->m_id;
			}
			c->addComponent<OnlineOwnerComponent>(playerCompID);
			c->addComponent<RenderInActiveGameComponent>();

		}

		if (c->hasComponent<CandleComponent>()) {

			auto rec = c->addComponent<NetworkReceiverComponent>(torchCompID, Netcode::EntityType::CANDLE_ENTITY);
			if (NWrapperSingleton::getInstance().isHost()) {
				c->addComponent<NetworkSenderComponent>(Netcode::EntityType::CANDLE_ENTITY, torchCompID)->m_id = rec->m_id;
			}

			c->addComponent<OnlineOwnerComponent>(playerCompID);
			c->addComponent<RenderInActiveGameComponent>();
		}
	}

	if (NWrapperSingleton::getInstance().isHost()) {
		otherPlayer->addComponent<NetworkSenderComponent>(Netcode::EntityType::PLAYER_ENTITY, playerCompID, Netcode::MessageType::UPDATE_SANITY)->m_id = rec->m_id;
	}


	// REPLAY COPY OF THE ENTITY
	CreateReplayPlayer(playerCompID, torchCompID, gunCompID, lightIndex, spawnLocation);
}


// Creates a copy of the player but does not immediately include it in any systems until the killcam is started and
// KillCamReceiverSystem starts running.
Entity::SPtr EntityFactory::CreateReplayPlayer(Netcode::ComponentID playerCompID, Netcode::ComponentID candleCompID,
	Netcode::ComponentID gunCompID, size_t lightIndex, glm::vec3 spawnLocation) {
	Entity::SPtr replayPlayer = ECS::Instance()->createEntity("ReplayPlayer");
	replayPlayer->tryToAddToSystems = false;

	// replay players spawn under the map since if they die long before the killcam spawns they won't be removed until
	// the match ends
	CreateGenericPlayer(replayPlayer, lightIndex, { 0.f, -100.f, 0.f }, Netcode::getComponentOwner(playerCompID), true);

	replayPlayer->addComponent<ReplayReceiverComponent>(playerCompID, Netcode::EntityType::PLAYER_ENTITY);

	// Remove components that shouldn't be used by entities in the killcam
	replayPlayer->removeComponent<CollidableComponent>();
	replayPlayer->removeComponent<SpeedLimitComponent>();
	replayPlayer->removeComponent<SanityComponent>();
	replayPlayer->removeComponent<AudioComponent>(); // TODO: Remove this line when we start having audio in the killcam
	replayPlayer->removeComponent<GunComponent>();
	replayPlayer->removeComponent<BoundingBoxComponent>();


	// So that we can get their camera positions in the replay
	AnimationComponent* ac = replayPlayer->getComponent<AnimationComponent>();
	ac->is_camFollowingHead = true;
	ac->headPositionMatrix = glm::translate(glm::identity<glm::mat4>(), ac->headPositionLocalDefault);


	// Create the player
	AddCandleComponentsToPlayer(replayPlayer, lightIndex, Netcode::getComponentOwner(playerCompID));

	for (Entity* c : replayPlayer->getChildEntities()) {
		if (c->getName() == replayPlayer->getName() + "WaterGun") {
			c->addComponent<ReplayReceiverComponent>(gunCompID, Netcode::EntityType::GUN_ENTITY);
			ECS::Instance()->getSystem<KillCamReceiverSystem>()->instantAddEntity(c);
			c->removeComponent<BoundingBoxComponent>();
		}
		if (c->hasComponent<CandleComponent>()) {
			c->addComponent<ReplayReceiverComponent>(candleCompID, Netcode::EntityType::CANDLE_ENTITY);
			c->removeComponent<CollidableComponent>();
			c->removeComponent<BoundingBoxComponent>();
			ECS::Instance()->getSystem<KillCamReceiverSystem>()->instantAddEntity(c);
		}
	}

	ECS::Instance()->getSystem<KillCamReceiverSystem>()->instantAddEntity(replayPlayer.get());

	// Note: The RenderInReplayComponent is added to these entities once KillCamReceiverSystem start running so don't
	//       add those components here

	return replayPlayer;
}

void EntityFactory::CreatePerformancePlayer(Entity::SPtr playerEnt, size_t lightIndex, glm::vec3 spawnLocation) {
	static Netcode::PlayerID perfromancePlayerID = 100;

	CreateGenericPlayer(playerEnt, lightIndex, spawnLocation, 0);
	Netcode::ComponentID playerCompID = playerEnt->addComponent<NetworkSenderComponent>(Netcode::EntityType::PLAYER_ENTITY, Netcode::PlayerID(100), Netcode::MessageType::ANIMATION)->m_id;
	playerEnt->addComponent<NetworkReceiverComponent>(playerCompID, Netcode::EntityType::PLAYER_ENTITY);
	playerEnt->addComponent<MovementComponent>();
	playerEnt->addComponent<RenderInActiveGameComponent>();

	// Create the player
	AddCandleComponentsToPlayer(playerEnt, lightIndex, 0);


	for (Entity* c : playerEnt->getChildEntities()) {
		if (c->getName() == playerEnt->getName() + "WaterGun") {
			c->addComponent<RenderInActiveGameComponent>();
		}
		if (c->hasComponent<CandleComponent>()) {
			c->addComponent<RenderInActiveGameComponent>();
		}
	}


	perfromancePlayerID++;
}

Entity::SPtr EntityFactory::CreateMySpectator(Netcode::PlayerID playerID, size_t lightIndex, glm::vec3 spawnLocation) {

	auto mySpectator = ECS::Instance()->createEntity("MyPlayer");

	mySpectator->addComponent<TransformComponent>(spawnLocation);
	mySpectator->addComponent<SpectatorComponent>();
	mySpectator->addComponent<RenderInActiveGameComponent>();

	auto transform = mySpectator->getComponent<TransformComponent>();
	auto pos = glm::vec3(transform->getCurrentTransformState().m_translation);
	pos.y = 40.f;
	transform->setStartTranslation(pos * 0.5f);

	return mySpectator;
}

// Creates a player entity without a candle and without a model
void EntityFactory::CreateGenericPlayer(Entity::SPtr playerEntity, size_t lightIndex, glm::vec3 spawnLocation, Netcode::PlayerID playerID, bool doNotAddToSystems) {
	std::string modelName = "Doc.fbx";
	auto* shader = &Application::getInstance()->getResourceManager().getShaderSet<GBufferOutShader>();
	Model* characterModel = &Application::getInstance()->getResourceManager().getModelCopy(modelName, shader);
	characterModel->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Doc/Doc_MRAO.dds");
	characterModel->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Doc/Doc_Albedo.dds");
	characterModel->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Doc/Doc_NM.dds");
	characterModel->setIsAnimated(true);
	AnimationStack* stack = &Application::getInstance()->getResourceManager().getAnimationStack(modelName);

	// All players have a bounding box
	auto* wireframeShader = &Application::getInstance()->getResourceManager().getShaderSet<GBufferWireframe>();
	Model* boundingBoxModel = &Application::getInstance()->getResourceManager().getModel("boundingBox.fbx", wireframeShader);
	boundingBoxModel->getMesh(0)->getMaterial()->setColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	boundingBoxModel->getMesh(0)->getMaterial()->setAOScale(0.5);
	boundingBoxModel->getMesh(0)->getMaterial()->setMetalnessScale(0.5);
	boundingBoxModel->getMesh(0)->getMaterial()->setRoughnessScale(0.5);

	playerEntity->addComponent<PlayerComponent>();
	playerEntity->addComponent<TransformComponent>(spawnLocation);
	playerEntity->addComponent<CullingComponent>();
	playerEntity->addComponent<ModelComponent>(characterModel);
	playerEntity->addComponent<CollidableComponent>();
	playerEntity->addComponent<SpeedLimitComponent>()->maxSpeed = 6.0f;
	playerEntity->addComponent<SanityComponent>()->sanity = 100.0f;
	playerEntity->addComponent<SprintingComponent>();
	playerEntity->addComponent<PowerUpComponent>();

	// Give playerEntity a bounding box
	playerEntity->addComponent<BoundingBoxComponent>(boundingBoxModel);
	playerEntity->getComponent<BoundingBoxComponent>()->getBoundingBox()->setHalfSize(glm::vec3(0.4f, .9f, 0.4f)); // Needed?

	// Adding audio component and adding all sounds attached to the playerEntity entity
	playerEntity->addComponent<AudioComponent>();

	playerEntity->addComponent<GunComponent>();
	playerEntity->getComponent<TransformComponent>()->setStartTranslation(glm::vec3(0, 0.9f, 0) + spawnLocation);

	//give players teams
	playerEntity->addComponent<TeamComponent>()->team = NWrapperSingleton::getInstance().getPlayer(playerID)->team;


	AnimationComponent* ac = playerEntity->addComponent<AnimationComponent>(stack);
	ac->currentAnimation = stack->getAnimation(1);


	auto candle = ECS::Instance()->createEntity(playerEntity->getName() + "Candle");
	if (doNotAddToSystems) {
		candle->tryToAddToSystems = false;
	}
	CreateCandle(candle, glm::vec3(3.f, 1.f, 1.f), lightIndex);
	playerEntity->addChildEntity(candle.get());

	// Attach the something to the player's right hand
	ac->rightHandEntity = candle.get();
	ac->rightHandPosition = glm::identity<glm::mat4>();
	ac->rightHandPosition = glm::translate(ac->rightHandPosition, glm::vec3(-0.596f, 1.026f, 0.055f));
	ac->rightHandPosition = ac->rightHandPosition * glm::toMat4(glm::quat(glm::vec3(1.178f, 0.646f, -0.300f)));

//	auto gun = CreateWaterGun(playerEntity->getName() + "WaterGun");
//	if (doNotAddToSystems) {
//		gun->tryToAddToSystems = false;
//	}
//	playerEntity->addChildEntity(gun.get());
//
//	// Attach the candle to the player's left hand
//	ac->leftHandEntity = gun.get();
//	ac->leftHandPosition = glm::identity<glm::mat4>();
//	ac->leftHandPosition = glm::translate(ac->leftHandPosition, glm::vec3(0.563f, 1.059f, 0.110f));
//	ac->leftHandPosition = ac->leftHandPosition * glm::toMat4(glm::quat(glm::vec3(1.178f, -0.462f, 0.600f)));
}

Entity::SPtr EntityFactory::CreatePowerUp(glm::vec3& spawn, const int type, Netcode::ComponentID comID) {
	auto* shader = &Application::getInstance()->getResourceManager().getShaderSet<GBufferOutShader>();
	Model* powerUpModel = &Application::getInstance()->getResourceManager().getModel("Clutter/PowerUp.fbx", shader);
	powerUpModel->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Clutter/powerUp_MRAO.dds");
	powerUpModel->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Clutter/powerUp.dds");

	auto powerUp = ECS::Instance()->createEntity("powerup" + Utils::toStr(spawn) + ":" + std::to_string(type));
	powerUp->addComponent<ModelComponent>(powerUpModel)->teamColorID = type;
	
	powerUp->addComponent<TransformComponent>(spawn);
	powerUp->addComponent<RenderInActiveGameComponent>();
	powerUp->addComponent<PowerUpCollectibleComponent>()->powerUp = type;
	
	if (NWrapperSingleton::getInstance().isHost()) {
		Netcode::ComponentID id = powerUp->addComponent<NetworkSenderComponent>(Netcode::EntityType::POWER_UP, Netcode::NEUTRAL_OWNER_ID)->m_id;
		powerUp->addComponent<NetworkReceiverComponent>(id, Netcode::EntityType::POWER_UP);
	} else {
		powerUp->addComponent<NetworkReceiverComponent>(comID, Netcode::EntityType::POWER_UP);
	}

	auto* moveC = powerUp->addComponent<MovementComponent>();
	moveC->rotation = glm::vec3(1, 1, 1);
	
	return powerUp;
}


Entity::SPtr EntityFactory::CreateBot(Model* boundingBoxModel, Model* characterModel, const glm::vec3& pos, Model* lightModel, size_t lightIndex, NodeSystem* ns) {

	auto e = ECS::Instance()->createEntity("AiCharacter");
	e->addComponent<ModelComponent>(characterModel);
	e->addComponent<TransformComponent>(pos);
	e->addComponent<BoundingBoxComponent>(boundingBoxModel)->getBoundingBox()->setHalfSize(glm::vec3(0.7f, .9f, 0.7f));
	e->addComponent<CollidableComponent>(true);
	e->addComponent<MovementComponent>();
	e->addComponent<SpeedLimitComponent>();
	e->addComponent<CollisionComponent>(true);
	e->addComponent<AiComponent>();
	e->addComponent<CullingComponent>();

	e->addComponent<AudioComponent>();

	e->getComponent<MovementComponent>()->constantAcceleration = glm::vec3(0.0f, -9.8f, 0.0f);
	e->getComponent<SpeedLimitComponent>()->maxSpeed = 3.0f;

	e->addComponent<GunComponent>();

	auto aiCandleEntity = ECS::Instance()->createEntity("AiCandle");
	EntityFactory::CreateCandle(aiCandleEntity, glm::vec3(0.f, 2.f, 0.f), lightIndex);

	e->addChildEntity(aiCandleEntity.get());
	auto fsmComp = e->addComponent<FSMComponent>();

	// =========Create states and transitions===========

	SearchingState* searchState = fsmComp->createState<SearchingState>(ns);
	AttackingState* attackState = fsmComp->createState<AttackingState>();
	fsmComp->createState<FleeingState>(ns);

	// TODO: unnecessary to create new transitions for each FSM if they're all identical
	//Attack State
	FSM::Transition* attackToFleeing = SAIL_NEW FSM::Transition;
	attackToFleeing->addBoolCheck(&aiCandleEntity->getComponent<CandleComponent>()->isLit, false);
	FSM::Transition* attackToSearch = SAIL_NEW FSM::Transition;
	attackToSearch->addFloatGreaterThanCheck(attackState->getDistToHost(), 100.0f);

	// Search State
	FSM::Transition* searchToAttack = SAIL_NEW FSM::Transition;
	searchToAttack->addFloatLessThanCheck(searchState->getDistToHost(), 100.0f);
	FSM::Transition* searchToFleeing = SAIL_NEW FSM::Transition;
	searchToFleeing->addBoolCheck(&aiCandleEntity->getComponent<CandleComponent>()->isLit, false);

	// Fleeing State
	FSM::Transition* fleeingToSearch = SAIL_NEW FSM::Transition;
	fleeingToSearch->addBoolCheck(&aiCandleEntity->getComponent<CandleComponent>()->isLit, true);

	fsmComp->addTransition<AttackingState, FleeingState>(attackToFleeing);
	fsmComp->addTransition<AttackingState, SearchingState>(attackToSearch);

	fsmComp->addTransition<SearchingState, AttackingState>(searchToAttack);
	fsmComp->addTransition<SearchingState, FleeingState>(searchToFleeing);

	fsmComp->addTransition<FleeingState, SearchingState>(fleeingToSearch);
	// =========[END] Create states and transitions===========


	return e;
}

Entity::SPtr EntityFactory::CreateCleaningBot(const glm::vec3& pos, NodeSystem* ns) {
	auto e = ECS::Instance()->createEntity("Cleaning Bot");

	const Netcode::ComponentID compID = Netcode::generateUniqueBotID();

	std::string modelName = "CleaningBot.fbx";
	Model* botModel = &Application::getInstance()->getResourceManager().getModelCopy(modelName, &Application::getInstance()->getResourceManager().getShaderSet<GBufferOutShader>());
	botModel->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/CleaningRobot/CleaningBot_MRAO.dds");
	botModel->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/CleaningRobot/CleaningBot_Albedo.dds");
	botModel->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/CleaningRobot/CleaningBot_NM.dds");
	botModel->setIsAnimated(false);

	e->addComponent<ModelComponent>(botModel);
	e->addComponent<TransformComponent>(pos);
	if (NWrapperSingleton::getInstance().isHost()) {
		auto sendC = e->addComponent<NetworkSenderComponent>(Netcode::EntityType::MECHA_ENTITY, compID);
		sendC->addMessageType(Netcode::MessageType::CHANGE_LOCAL_POSITION);
		sendC->addMessageType(Netcode::MessageType::CHANGE_LOCAL_ROTATION);
	}
	e->addComponent<NetworkReceiverComponent>(compID, Netcode::EntityType::MECHA_ENTITY);
	e->addComponent<MovementComponent>();
	e->addComponent<SpeedLimitComponent>();
	e->addComponent<CollisionComponent>(true);
	e->addComponent<AiComponent>();
	e->addComponent<CullingComponent>();
	e->addComponent<RenderInActiveGameComponent>();

	e->addComponent<AudioComponent>();

	e->getComponent<MovementComponent>()->constantAcceleration = glm::vec3(0.0f, 0.f, 0.0f);
	e->getComponent<SpeedLimitComponent>()->maxSpeed = 2.0f;

	auto fsmComp = e->addComponent<FSMComponent>();

	// =========Create states and transitions===========

	SearchingState* searchState = fsmComp->createState<SearchingState>(ns);
	// Keep this for now

	//AttackingState* attackState = fsmComp->createState<AttackingState>();
	//fsmComp->createState<FleeingState>(ns);

	// TODO: unnecessary to create new transitions for each FSM if they're all identical
	//Attack State
	/*FSM::Transition* attackToFleeing = SAIL_NEW FSM::Transition;
	attackToFleeing->addBoolCheck(&aiCandleEntity->getComponent<CandleComponent>()->isLit, false);
	FSM::Transition* attackToSearch = SAIL_NEW FSM::Transition;
	attackToSearch->addFloatGreaterThanCheck(attackState->getDistToHost(), 100.0f);

	// Search State
	FSM::Transition* searchToAttack = SAIL_NEW FSM::Transition;
	searchToAttack->addFloatLessThanCheck(searchState->getDistToHost(), 100.0f);
	FSM::Transition* searchToFleeing = SAIL_NEW FSM::Transition;
	searchToFleeing->addBoolCheck(&aiCandleEntity->getComponent<CandleComponent>()->isLit, false);

	// Fleeing State
	FSM::Transition* fleeingToSearch = SAIL_NEW FSM::Transition;
	fleeingToSearch->addBoolCheck(&aiCandleEntity->getComponent<CandleComponent>()->isLit, true);

	//fsmComp->addTransition<AttackingState, FleeingState>(attackToFleeing);
	//fsmComp->addTransition<AttackingState, SearchingState>(attackToSearch);

	fsmComp->addTransition<SearchingState, AttackingState>(searchToAttack);
	fsmComp->addTransition<SearchingState, FleeingState>(searchToFleeing);

	fsmComp->addTransition<FleeingState, SearchingState>(fleeingToSearch);*/
	// =========[END] Create states and transitions===========

	return e;
}

static int handle2 = 0;
Entity::SPtr EntityFactory::CreateStaticMapObject(const std::string& name, Model* model, Model* boundingBoxModel, const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scale) {
	std::string usedName = name;
	if (name == "ClutterSpecial") {
		if (handle2 < 2) {
			usedName = "CloningVat";
			handle2++;
		}
	}
	
	
	auto e = ECS::Instance()->createEntity(usedName);
	e->addComponent<ModelComponent>(model);
	e->addComponent<TransformComponent>(pos, rot, scale);
	e->addComponent<BoundingBoxComponent>(boundingBoxModel);
	e->addComponent<CollidableComponent>();
	e->addComponent<CullingComponent>();

	e->addComponent<RenderInActiveGameComponent>();

	// Components needed to be rendered in the killcam
	e->addComponent<RenderInReplayComponent>();

	return e;
}

Entity::SPtr EntityFactory::CreateProjectile(Entity::SPtr e, const EntityFactory::ProjectileArguments& info) {
	constexpr float radius = 0.075f; // the radius of the projectile's hitbox (in meters)

	e->addComponent<MetaballComponent>()->renderGroupIndex = info.ownersNetId;
	e->addComponent<BoundingBoxComponent>()->getBoundingBox()->setHalfSize(glm::vec3(radius, radius, radius));
	e->addComponent<LifeTimeComponent>(info.lifetime);
	e->addComponent<ProjectileComponent>(10.0f, info.hasLocalOwner); // TO DO should not be manually set to true
	e->getComponent<ProjectileComponent>()->ownedBy = info.ownersNetId;
	e->addComponent<TransformComponent>(info.pos);


	if (info.hasLocalOwner == true) {
		e->addComponent<LocalOwnerComponent>(info.ownersNetId);
		e->addComponent<NetworkSenderComponent>(Netcode::EntityType::PROJECTILE_ENTITY, info.netCompId);
	} else {
		e->addComponent<OnlineOwnerComponent>(info.ownersNetId);
	}
	e->addComponent<NetworkReceiverComponent>(info.netCompId, Netcode::EntityType::PROJECTILE_ENTITY);


	MovementComponent* movement = e->addComponent<MovementComponent>();
	movement->velocity = info.velocity;
	movement->constantAcceleration = glm::vec3(0.f, -9.8f, 0.f);

	CollisionComponent* collision = e->addComponent<CollisionComponent>();
	collision->drag = 15.0f;
	// NOTE: 0.0f <= Bounciness <= 1.0f
	collision->bounciness = 0.0f;
	collision->padding = radius;

	e->addComponent<RenderInActiveGameComponent>();

	return e;
}

Entity::SPtr EntityFactory::CreateReplayProjectile(Entity::SPtr e, const ProjectileArguments& info) {
	e->addComponent<MetaballComponent>()->renderGroupIndex = info.ownersNetId;
	e->addComponent<BoundingBoxComponent>()->getBoundingBox()->setHalfSize(glm::vec3(0.15, 0.15, 0.15));
	e->addComponent<LifeTimeComponent>(info.lifetime);

	e->addComponent<TransformComponent>(info.pos);
	e->addComponent<ReplayReceiverComponent>(info.netCompId, Netcode::EntityType::PROJECTILE_ENTITY);

	MovementComponent* movement = e->addComponent<MovementComponent>();
	movement->velocity = info.velocity;
	movement->constantAcceleration = glm::vec3(0.f, -9.8f, 0.f);

	e->addComponent<RenderInReplayComponent>();

	return e;
}

Entity::SPtr EntityFactory::CreateScreenSpaceText(const std::string& text, glm::vec2 origin, glm::vec2 size) {
	static int num = 0;

	auto GUIEntity = ECS::Instance()->createEntity("TextEntity:" + text);
	ModelFactory::StringModel::Constraints textConst;
	textConst.origin = Mesh::vec3(origin.x, origin.y, 0.f);
	textConst.size = Mesh::vec2(size.x, size.y);
	textConst.text = text;
	auto GUIModel = ModelFactory::StringModel::Create(&Application::getInstance()->getResourceManager().getShaderSet<GuiShader>(), textConst);
	std::string modelName = "TextModel " + std::to_string(num);
	Application::getInstance()->getResourceManager().addModel(modelName, GUIModel);
	for (UINT i = 0; i < GUIModel->getNumberOfMeshes(); i++) {
		GUIModel->getMesh(i)->getMaterial()->setAlbedoTexture(GUIText::fontTexture);
	}
	GUIEntity->addComponent<GUIComponent>(&Application::getInstance()->getResourceManager().getModel(modelName));
	num++;

	return GUIEntity;
}

Entity::SPtr EntityFactory::CreateGUIEntity(const std::string& name, const std::string& texture, glm::vec2 origin, glm::vec2 size) {
	auto ent = ECS::Instance()->createEntity(name);
	ModelFactory::QuadModel::Constraints entConst;
	entConst.origin = Mesh::vec3(origin.x, origin.y, 0.f);
	entConst.halfSize = Mesh::vec2(size.x, size.y);
	auto entModel = ModelFactory::QuadModel::Create(&Application::getInstance()->getResourceManager().getShaderSet<GuiShader>(), entConst);
	if (!Application::getInstance()->getResourceManager().hasTexture(texture)) {
		Application::getInstance()->getResourceManager().loadTexture(texture);
	}
	entModel->getMesh(0)->getMaterial()->setAlbedoTexture(texture);
	Application::getInstance()->getResourceManager().addModel(name + "Model", entModel);
	ent->addComponent<GUIComponent>(&Application::getInstance()->getResourceManager().getModel(name + "Model"));

	return ent;
}

Entity::SPtr EntityFactory::CreateCrosshairEntity(const std::string& name) {
	auto entity = ECS::Instance()->createEntity(name);

	entity->addComponent<CrosshairComponent>();

	return entity;
}
