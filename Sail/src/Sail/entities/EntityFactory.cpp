#include "pch.h"
#include "EntityFactory.hpp"

#include "Sail/Application.h"
#include "Sail/graphics/geometry/Model.h"
#include "Sail/entities/components/Components.h"
#include "Sail/ai/pathfinding/NodeSystem.h"
#include "Sail/ai/states/AttackingState.h"
#include "Sail/ai/states/FleeingState.h"
#include "Sail/ai/states/SearchingState.h"
#include "Sail/entities/components/LocalOwnerComponent.h"
#include "Sail/entities/components/OnlineOwnerComponent.h"
#include "../Sail/src/Network/NWrapperSingleton.h"
#include "Sail/graphics/geometry/factory/StringModel.h"
#include "Sail/graphics/geometry/factory/QuadModel.h"
#include "Sail/entities/components/GUIComponent.h"
#include "Sail/entities/components/CrosshairHitComponent.h"

void EntityFactory::CreateCandle(Entity::SPtr& candle, const glm::vec3& lightPos, size_t lightIndex) {
	// Candle has a model and a bounding box
	auto* shader = &Application::getInstance()->getResourceManager().getShaderSet<GBufferOutShader>();
	Model* candleModel = &Application::getInstance()->getResourceManager().getModel("Torch.fbx", shader);
	candleModel->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Torch/Torch_Albedo.tga");
	candleModel->getMesh(0)->getMaterial()->setNormalTexture("pbr/Torch/Torch_NM.tga");
	candleModel->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Torch/Torch_MRAO.tga");


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

#ifdef DEVELOPMENT
	auto* particleEmitterComp = candle->addComponent<ParticleEmitterComponent>();
	particleEmitterComp->position = { 4.0f, 1.0f, 0.0f };
	particleEmitterComp->velocity = { 0.0f, 0.2f, 0.0f };
	particleEmitterComp->acceleration = { 0.0f, 0.2f, 0.0f };
	particleEmitterComp->spread = { 0.3f, 0.5f, 0.3f };
	particleEmitterComp->spawnRate = 0.01f;
	particleEmitterComp->lifeTime = 1.0f;
#endif

	PointLight pl;
	pl.setColor(glm::vec3(0.55f, 0.5f, 0.45f));
	pl.setPosition(glm::vec3(lightPos.x, lightPos.y + .5f, lightPos.z));
	pl.setAttenuation(0.f, 0.f, 0.2f);
	pl.setIndex(lightIndex);
	candle->addComponent<LightComponent>(pl);
}

Entity::SPtr EntityFactory::CreateWaterGun(const std::string& name) {

	auto* shader = &Application::getInstance()->getResourceManager().getShaderSet<GBufferOutShader>();
	Model* candleModel = &Application::getInstance()->getResourceManager().getModel("WaterPistol.fbx", shader);
	candleModel->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/WaterGun/Watergun_Albedo.tga");
	candleModel->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/WaterGun/Watergun_MRAO.tga");
	candleModel->getMesh(0)->getMaterial()->setNormalTexture("pbr/WaterGun/Watergun_NM.tga");


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
			c->addComponent<TeamComponent>()->team = (playerID) % 12;
		}
	}
}

Entity::SPtr EntityFactory::CreateMyPlayer(Netcode::PlayerID playerID, size_t lightIndex, glm::vec3 spawnLocation) {
	// Create my player
	auto myPlayer = ECS::Instance()->createEntity("MyPlayer");
	EntityFactory::CreateGenericPlayer(myPlayer, lightIndex, spawnLocation,playerID);

	auto senderC = myPlayer->addComponent<NetworkSenderComponent>(Netcode::EntityType::PLAYER_ENTITY, playerID, Netcode::MessageType::ANIMATION);
	senderC->addMessageType(Netcode::MessageType::CHANGE_LOCAL_POSITION);
	senderC->addMessageType(Netcode::MessageType::CHANGE_LOCAL_ROTATION);

	Netcode::ComponentID netComponentID = myPlayer->getComponent<NetworkSenderComponent>()->m_id;
	myPlayer->addComponent<NetworkReceiverComponent>(netComponentID, Netcode::EntityType::PLAYER_ENTITY);
	myPlayer->addComponent<ReplayComponent>(netComponentID, Netcode::EntityType::PLAYER_ENTITY);
	myPlayer->addComponent<LocalOwnerComponent>(netComponentID);
	myPlayer->addComponent<CollisionComponent>();
	myPlayer->getComponent<ModelComponent>()->renderToGBuffer = false;
	myPlayer->addComponent<MovementComponent>()->constantAcceleration = glm::vec3(0.0f, -9.8f, 0.0f);
	myPlayer->addComponent<RealTimeComponent>();
	myPlayer->addComponent<SprintingComponent>();

#ifdef DEVELOPMENT
	//For testing, add particle emitter to player.
	auto* particleEmitterComp = myPlayer->addComponent<ParticleEmitterComponent>();
	particleEmitterComp->position = { 0.0f, 2.0f, 0.0f };
	particleEmitterComp->velocity = {0.0f, 4.0f, 0.0f};
	particleEmitterComp->acceleration = { 0.0f, -9.8f, 0.0f };
	particleEmitterComp->spread = {2.0f, 2.0f, 1.0f};
	particleEmitterComp->spawnRate = 0.001f;
	particleEmitterComp->lifeTime = 1.0f;
#endif

	AnimationComponent* ac = myPlayer->getComponent<AnimationComponent>();

	AddCandleComponentsToPlayer(myPlayer, lightIndex, playerID);

	Netcode::ComponentID candleNetID;
	Netcode::ComponentID gunNetID;

	for (Entity* c : myPlayer->getChildEntities()) {
		if (c->getName() == myPlayer->getName() + "WaterGun") {
			gunNetID = c->addComponent<NetworkSenderComponent>(Netcode::EntityType::GUN_ENTITY, playerID)->m_id;
			c->addComponent<NetworkReceiverComponent>(gunNetID, Netcode::EntityType::GUN_ENTITY);
			c->addComponent<ReplayComponent>(gunNetID, Netcode::EntityType::GUN_ENTITY);
			//leave this for now
			//c->addComponent<GunComponent>();]
			c->addComponent<RealTimeComponent>(); // The player's gun is updated each frame
		}

		// Add a localOwnerComponent to our candle so that we can differentiate it from other candles
		if (c->hasComponent<CandleComponent>()) {
			candleNetID = c->addComponent<NetworkSenderComponent>(Netcode::EntityType::CANDLE_ENTITY, playerID)->m_id;
			c->addComponent<NetworkReceiverComponent>(candleNetID, Netcode::EntityType::CANDLE_ENTITY);
			c->addComponent<ReplayComponent>(candleNetID, Netcode::EntityType::CANDLE_ENTITY);
			c->addComponent<LocalOwnerComponent>(netComponentID);
			c->addComponent<RealTimeComponent>(); // The player's candle is updated each frame
		}
	}

	// For debugging
	SAIL_LOG("My netcompID: " + std::to_string(netComponentID));

	// Tell other players to create my character
	NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
		Netcode::MessageType::CREATE_NETWORKED_PLAYER,
		SAIL_NEW Netcode::MessageCreatePlayer{
			netComponentID,
			candleNetID,
			gunNetID,
			myPlayer->getComponent<TransformComponent>()->getCurrentTransformState().m_translation,
		},
		false
		);

	return myPlayer;
}

// otherPlayer is an entity that doesn't have any components added to it yet.
// Needed so that NetworkReceiverSystem can add the entity to itself before 
void EntityFactory::CreateOtherPlayer(Entity::SPtr otherPlayer, 
	Netcode::ComponentID playerCompID, 
	Netcode::ComponentID candleCompID, 
	Netcode::ComponentID gunCompID, 
	size_t lightIndex, glm::vec3 spawnLocation) 
{
	EntityFactory::CreateGenericPlayer(otherPlayer, lightIndex, spawnLocation, Netcode::getComponentOwner(playerCompID));
	// Other players have a character model and animations

	otherPlayer->addComponent<NetworkReceiverComponent>(playerCompID, Netcode::EntityType::PLAYER_ENTITY);
	otherPlayer->addComponent<ReplayComponent>(playerCompID, Netcode::EntityType::PLAYER_ENTITY);
	otherPlayer->addComponent<OnlineOwnerComponent>(playerCompID);

	// Create the player
	AddCandleComponentsToPlayer(otherPlayer, lightIndex, Netcode::getComponentOwner(playerCompID));

	for (Entity* c : otherPlayer->getChildEntities()) {
		if (c->getName() == otherPlayer->getName() + "WaterGun") {
			c->addComponent<NetworkReceiverComponent>(gunCompID, Netcode::EntityType::GUN_ENTITY);
			c->addComponent<ReplayComponent>(gunCompID, Netcode::EntityType::GUN_ENTITY);
			c->addComponent<OnlineOwnerComponent>(playerCompID);
		}

		if (c->hasComponent<CandleComponent>()) {
			c->addComponent<NetworkReceiverComponent>(candleCompID, Netcode::EntityType::CANDLE_ENTITY);
			c->addComponent<ReplayComponent>(candleCompID, Netcode::EntityType::CANDLE_ENTITY);
			c->addComponent<OnlineOwnerComponent>(playerCompID);
		}
	}
}

void EntityFactory::CreatePerformancePlayer(Entity::SPtr playerEnt, size_t lightIndex, glm::vec3 spawnLocation) {

	static Netcode::PlayerID perfromancePlayerID = 100;


	CreateGenericPlayer(playerEnt, lightIndex, spawnLocation,0);
	Netcode::ComponentID playerCompID = playerEnt->addComponent<NetworkSenderComponent>(Netcode::EntityType::PLAYER_ENTITY, Netcode::PlayerID(100), Netcode::MessageType::ANIMATION)->m_id;
	playerEnt->addComponent<NetworkReceiverComponent>(playerCompID, Netcode::EntityType::PLAYER_ENTITY);
	playerEnt->addComponent<MovementComponent>();


	// Create the player
	AddCandleComponentsToPlayer(playerEnt, lightIndex, 0);

	perfromancePlayerID++;
}

// Creates a player enitty without a candle and without a model
void EntityFactory::CreateGenericPlayer(Entity::SPtr playerEntity, size_t lightIndex, glm::vec3 spawnLocation, Netcode::PlayerID playerID) {
	
	std::string modelName = "Doc.fbx";
	auto* shader = &Application::getInstance()->getResourceManager().getShaderSet<GBufferOutShader>();
	Model* characterModel = &Application::getInstance()->getResourceManager().getModelCopy(modelName, shader);
	characterModel->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Character/CharacterMRAO.tga");
	characterModel->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Character/CharacterTex.tga");
	characterModel->getMesh(0)->getMaterial()->setNormalTexture("pbr/Character/CharacterNM.tga");
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


	// Give playerEntity a bounding box
	playerEntity->addComponent<BoundingBoxComponent>(boundingBoxModel);
	playerEntity->getComponent<BoundingBoxComponent>()->getBoundingBox()->setHalfSize(glm::vec3(0.4f, .9f, 0.4f)); // Needed?

	// Adding audio component and adding all sounds attached to the playerEntity entity
	playerEntity->addComponent<AudioComponent>();

	playerEntity->addComponent<GunComponent>();
	playerEntity->getComponent<TransformComponent>()->setStartTranslation(glm::vec3(1.6f, 0.9f, 1.f) + spawnLocation);

	//give players teams
	playerEntity->addComponent<TeamComponent>()->team = (playerID) % 12;

	AnimationComponent* ac = playerEntity->addComponent<AnimationComponent>(stack);
	ac->currentAnimation = stack->getAnimation(1);



	auto candle = ECS::Instance()->createEntity(playerEntity->getName() + "Candle");
	CreateCandle(candle, glm::vec3(0.f, 0.f, 0.f), lightIndex);
	playerEntity->addChildEntity(candle.get());

	// Attach the something to the player's right hand
	ac->rightHandEntity = candle.get();
	ac->rightHandPosition = glm::identity<glm::mat4>();
	ac->rightHandPosition = glm::translate(ac->rightHandPosition, glm::vec3(-0.596f, 1.026f, 0.055f));
	ac->rightHandPosition = ac->rightHandPosition * glm::toMat4(glm::quat(glm::vec3(1.178f, 0.646f, -0.300f)));

	auto gun = CreateWaterGun(playerEntity->getName() + "WaterGun");
	playerEntity->addChildEntity(gun.get());

	// Attach the candle to the player's left hand
	ac->leftHandEntity = gun.get();
	ac->leftHandPosition = glm::identity<glm::mat4>();
	ac->leftHandPosition = glm::translate(ac->leftHandPosition, glm::vec3(0.563f, 1.059f, 0.110f));
	ac->leftHandPosition = ac->leftHandPosition * glm::toMat4(glm::quat(glm::vec3(1.178f, -0.462f, 0.600f)));

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

Entity::SPtr EntityFactory::CreateStaticMapObject(const std::string& name, Model* model, Model* boundingBoxModel, const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scale) {
	auto e = ECS::Instance()->createEntity(name);
	e->addComponent<ModelComponent>(model);
	e->addComponent<TransformComponent>(pos, rot, scale);
	e->addComponent<BoundingBoxComponent>(boundingBoxModel);
	e->addComponent<CollidableComponent>();
	e->addComponent<CullingComponent>();

	return e;
}

Entity::SPtr EntityFactory::CreateProjectile(Entity::SPtr e,
		const glm::vec3& pos, const glm::vec3& velocity, 
		bool hasLocalOwner, Netcode::ComponentID ownersNetId, 
		Netcode::ComponentID netCompId, float lifetime) 
{
	e->addComponent<MetaballComponent>();
	e->addComponent<BoundingBoxComponent>()->getBoundingBox()->setHalfSize(glm::vec3(0.15, 0.15, 0.15));
	e->addComponent<LifeTimeComponent>(lifetime);
	e->addComponent<ProjectileComponent>(10.0f, hasLocalOwner); // TO DO should not be manually set to true
	e->getComponent<ProjectileComponent>()->ownedBy = ownersNetId;
	e->addComponent<TransformComponent>(pos);
	
	if (hasLocalOwner == true) {
		e->addComponent<LocalOwnerComponent>(ownersNetId);
		e->addComponent<NetworkSenderComponent>(Netcode::EntityType::PROJECTILE_ENTITY, netCompId);
	} else {
		e->addComponent<OnlineOwnerComponent>(ownersNetId);
	}
	e->addComponent<NetworkReceiverComponent>(netCompId, Netcode::EntityType::PROJECTILE_ENTITY);
	

	MovementComponent* movement = e->addComponent<MovementComponent>();
	movement->velocity = velocity;
	movement->constantAcceleration = glm::vec3(0.f, -9.8f, 0.f);

	CollisionComponent* collision = e->addComponent<CollisionComponent>();
	collision->drag = 15.0f;
	// NOTE: 0.0f <= Bounciness <= 1.0f
	collision->bounciness = 0.0f;
	collision->padding = 0.15f;

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
	for (int i = 0; i < GUIModel->getNumberOfMeshes(); i++) {
		GUIModel->getMesh(i)->getMaterial()->setAlbedoTexture(GUIText::fontTexture);
	}
	GUIEntity->addComponent<GUIComponent>(*&Application::getInstance()->getResourceManager().getModel(modelName));
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
	ent->addComponent<GUIComponent>(
		Application::getInstance()->getResourceManager().getModel(name + "Model"))
	);

	return ent;
}

Entity::SPtr EntityFactory::CreateCrosshairEntity(const std::string& name, const std::string& normalTexture, const std::string& alteredTexture, glm::vec2 origin, glm::vec2 size) {
	Entity::SPtr entity = ECS::Instance()->createEntity(name);

	// Size and location of both normal/altered models
	ModelFactory::QuadModel::Constraints constraints;
	constraints.origin = Mesh::vec3(origin.x, origin.y, 0.f);
	constraints.halfSize = Mesh::vec2(size.x, size.y);

	// Set up normal model
	{
		// Model
		Model* normalModel = ModelFactory::QuadModel::Create(
			&Application::getInstance()->getResourceManager().getShaderSet<GuiShader>(),
			constraints
		);
		// Texture
		if (!Application::getInstance()->getResourceManager().hasTexture(normalTexture)) {
			Application::getInstance()->getResourceManager().loadTexture(normalTexture);
		}
		normalModel->getMesh(0)->getMaterial()->setAlbedoTexture(normalTexture);
		// 
		Application::getInstance()->getResourceManager().addModel(name + "normalModel", normalModel);
	}
	
	// Set up altered crosshair model
	{
		// Model
		Model* alteredModel = ModelFactory::QuadModel::Create(
			&Application::getInstance()->getResourceManager().getShaderSet<GuiShader>(),
			constraints
		);
		// Texture
		if (!Application::getInstance()->getResourceManager().hasTexture(alteredTexture)) {
			Application::getInstance()->getResourceManager().loadTexture(alteredTexture);
		}
		alteredModel->getMesh(0)->getMaterial()->setAlbedoTexture(alteredTexture);
		//
		Application::getInstance()->getResourceManager().addModel(name + "alteredModel", alteredModel);
	}
	
	// Add CrosshairHitComponent to entity
	entity->addComponent<CrosshairHitComponent>(
		0.5,
		&Application::getInstance()->getResourceManager().getModel(name + "normalModel"),
		&Application::getInstance()->getResourceManager().getModel(name + "alteredModel")
	);

	// Add GUI Component to entity (and a pointer to the current model)
	entity->addComponent<GUIComponent>(
		&entity->getComponent<CrosshairHitComponent>()->normalModel
	);

	return entity;
}
