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

Entity::SPtr EntityFactory::CreateCandle(const std::string& name, Model* lightModel, Model* bbModel, const glm::vec3& lightPos, size_t lightIndex) {
	//creates light with model and pointlight
	auto e = ECS::Instance()->createEntity(name.c_str());
	e->addComponent<CandleComponent>();
	e->addComponent<ModelComponent>(lightModel);
	e->addComponent<TransformComponent>(lightPos);
	e->addComponent<BoundingBoxComponent>(bbModel);
	e->addComponent<CollidableComponent>();
	e->addComponent<CullingComponent>();
	PointLight pl;
	pl.setColor(glm::vec3(1.0f, 0.7f, 0.4f));
	pl.setPosition(glm::vec3(lightPos.x, lightPos.y + .37f, lightPos.z));
	pl.setAttenuation(0.f, 0.f, 0.2f);
	pl.setIndex(lightIndex);
	e->addComponent<LightComponent>(pl);

	return e;
}

Entity::SPtr EntityFactory::CreatePlayer(Model* boundingBoxModel, Model* projectileModel, Model* lightModel, unsigned char playerID, size_t lightIndex, glm::vec3 spawnLocation) {

	auto player = ECS::Instance()->createEntity("player");

	// TODO: Only used for AI, should be removed once AI can target player in a better way.
	//m_player = player.get();..

	// PlayerComponent is added to this entity to indicate that this is the player playing at this location, not a network connected player
	//player->addComponent<LocalPlayerComponent>();.

	player->addComponent<TransformComponent>();

	player->addComponent<CullingComponent>();

	player->addComponent<NetworkSenderComponent>(
		Netcode::MessageType::CREATE_NETWORKED_ENTITY,
		Netcode::EntityType::PLAYER_ENTITY,
		playerID
	);
	player->getComponent<NetworkSenderComponent>()->addDataType(Netcode::MessageType::ANIMATION);
	Netcode::ComponentID netComponentID = player->getComponent<NetworkSenderComponent>()->m_id;
	player->addComponent<NetworkReceiverComponent>(netComponentID, Netcode::EntityType::PLAYER_ENTITY);
	
	player->addComponent<LocalOwnerComponent>(netComponentID);

	// Add physics components and setting initial variables
	player->addComponent<MovementComponent>()->constantAcceleration = glm::vec3(0.0f, -9.8f, 0.0f);
	player->addComponent<SpeedLimitComponent>()->maxSpeed = 6.0f;
	player->addComponent<CollisionComponent>();

	// Give player a bounding box
	player->addComponent<BoundingBoxComponent>(boundingBoxModel);
	player->getComponent<BoundingBoxComponent>()->getBoundingBox()->setHalfSize(glm::vec3(0.7f, .9f, 0.7f));

	// Temporary projectile model for the player's gun
	player->addComponent<GunComponent>(projectileModel, boundingBoxModel);

	// Adding audio component and adding all sounds attached to the player entity
	player->addComponent<AudioComponent>();

#pragma region DEFINING PLAYER SOUNDS

	Audio::SoundInfo_General soundGeneralInfo{};
	Audio::SoundInfo_Unique soundUniqueInfo{};
	AudioComponent* playerAudio = player->getComponent<AudioComponent>();

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		 Running, Metal		 //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo.playOnce = false;
	soundGeneralInfo.positionalOffset = { 0.0f, -1.6f, 0.0f };
	playerAudio->defineSoundGeneral(Audio::SoundType::RUN_METAL, soundGeneralInfo);
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/footsteps_metal_1.wav";
		soundUniqueInfo.soundEffectLength = 0.919f;
		soundUniqueInfo.volume = 0.7f;
		playerAudio->defineSoundUnique(Audio::SoundType::RUN_METAL, soundUniqueInfo);
		//	• Sample #2
		soundUniqueInfo.fileName = "../Audio/footsteps_metal_2.wav";
		soundUniqueInfo.soundEffectLength = 0.941f;
		soundUniqueInfo.volume = 0.7f;
		playerAudio->defineSoundUnique(Audio::SoundType::RUN_METAL, soundUniqueInfo);
		//	• Sample #3
		soundUniqueInfo.fileName = "../Audio/footsteps_metal_3.wav";
		soundUniqueInfo.soundEffectLength = 0.921f;
		soundUniqueInfo.volume = 0.7f;
		playerAudio->defineSoundUnique(Audio::SoundType::RUN_METAL, soundUniqueInfo);
		//	• Sample #4
		soundUniqueInfo.fileName = "../Audio/footsteps_metal_4.wav";
		soundUniqueInfo.soundEffectLength = 0.936f;
		soundUniqueInfo.volume = 0.7f;
		playerAudio->defineSoundUnique(Audio::SoundType::RUN_METAL, soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		 Running, Tiles		 //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo.playOnce = false;
	soundGeneralInfo.positionalOffset = { 0.0f, -1.6f, 0.0f };
	playerAudio->defineSoundGeneral(Audio::SoundType::RUN_TILE, soundGeneralInfo);
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/footsteps_tile_1.wav";
		soundUniqueInfo.soundEffectLength = 0.919f;
		soundUniqueInfo.volume = 1.0f;
		playerAudio->defineSoundUnique(Audio::SoundType::RUN_TILE, soundUniqueInfo);
		//	• Sample #2
		soundUniqueInfo.fileName = "../Audio/footsteps_tile_2.wav";
		soundUniqueInfo.soundEffectLength = 0.941f;
		soundUniqueInfo.volume = 1.0f;
		playerAudio->defineSoundUnique(Audio::SoundType::RUN_TILE, soundUniqueInfo);
		//	• Sample #3
		soundUniqueInfo.fileName = "../Audio/footsteps_tile_3.wav";
		soundUniqueInfo.soundEffectLength = 0.921f;
		soundUniqueInfo.volume = 1.0f;
		playerAudio->defineSoundUnique(Audio::SoundType::RUN_TILE, soundUniqueInfo);
		//	• Sample #4
		soundUniqueInfo.fileName = "../Audio/footsteps_tile_4.wav";
		soundUniqueInfo.soundEffectLength = 0.936f;
		soundUniqueInfo.volume = 1.0f;
		playerAudio->defineSoundUnique(Audio::SoundType::RUN_TILE, soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		 Running, Water_Metal		 //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo.playOnce = false;
	soundGeneralInfo.positionalOffset = { 0.0f, -1.6f, 0.0f };
	playerAudio->defineSoundGeneral(Audio::SoundType::RUN_WATER_METAL, soundGeneralInfo);
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/footsteps_water_metal_1.wav";
		soundUniqueInfo.soundEffectLength = 0.919f;
		soundUniqueInfo.volume = 1.0f;
		playerAudio->defineSoundUnique(Audio::SoundType::RUN_WATER_METAL, soundUniqueInfo);
		//	• Sample #2
		soundUniqueInfo.fileName = "../Audio/footsteps_water_metal_2.wav";
		soundUniqueInfo.soundEffectLength = 0.941f;
		soundUniqueInfo.volume = 1.0f;
		playerAudio->defineSoundUnique(Audio::SoundType::RUN_WATER_METAL, soundUniqueInfo);
		//	• Sample #3
		soundUniqueInfo.fileName = "../Audio/footsteps_water_metal_3.wav";
		soundUniqueInfo.soundEffectLength = 0.921f;
		soundUniqueInfo.volume = 1.0f;
		playerAudio->defineSoundUnique(Audio::SoundType::RUN_WATER_METAL, soundUniqueInfo);
		//	• Sample #4
		soundUniqueInfo.fileName = "../Audio/footsteps_water_metal_4.wav";
		soundUniqueInfo.soundEffectLength = 0.936f;
		soundUniqueInfo.volume = 1.0f;
		playerAudio->defineSoundUnique(Audio::SoundType::RUN_WATER_METAL, soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		 Running, Water_Tiles		 //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo.playOnce = false;
	soundGeneralInfo.positionalOffset = { 0.0f, -1.6f, 0.0f };
	playerAudio->defineSoundGeneral(Audio::SoundType::RUN_WATER_TILE, soundGeneralInfo);
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/footsteps_water_tile_1.wav";
		soundUniqueInfo.soundEffectLength = 0.919f;
		soundUniqueInfo.volume = 1.0f;
		playerAudio->defineSoundUnique(Audio::SoundType::RUN_WATER_TILE, soundUniqueInfo);
		//	• Sample #2
		soundUniqueInfo.fileName = "../Audio/footsteps_water_tile_2.wav";
		soundUniqueInfo.soundEffectLength = 0.941f;
		soundUniqueInfo.volume = 1.0f;
		playerAudio->defineSoundUnique(Audio::SoundType::RUN_WATER_TILE, soundUniqueInfo);
		//	• Sample #3
		soundUniqueInfo.fileName = "../Audio/footsteps_water_tile_3.wav";
		soundUniqueInfo.soundEffectLength = 0.921f;
		soundUniqueInfo.volume = 1.0f;
		playerAudio->defineSoundUnique(Audio::SoundType::RUN_WATER_TILE, soundUniqueInfo);
		//	• Sample #4
		soundUniqueInfo.fileName = "../Audio/footsteps_water_tile_4.wav";
		soundUniqueInfo.soundEffectLength = 0.936f;
		soundUniqueInfo.volume = 1.0f;
		playerAudio->defineSoundUnique(Audio::SoundType::RUN_WATER_TILE, soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		   Watergun, Start		 //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo.playOnce = true;
	soundGeneralInfo.positionalOffset = { 0.0f, 0.0f, 0.0f };
	playerAudio->defineSoundGeneral(Audio::SoundType::SHOOT_START, soundGeneralInfo);
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/watergun_start.wav";
		soundUniqueInfo.soundEffectLength = 0.578f;
		soundUniqueInfo.volume = 0.7f;
		playerAudio->defineSoundUnique(Audio::SoundType::SHOOT_START, soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		   Watergun, Loop		 //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo.playOnce = false;
	soundGeneralInfo.positionalOffset = { 0.0f, 0.0f, 0.0f };
	playerAudio->defineSoundGeneral(Audio::SoundType::SHOOT_LOOP, soundGeneralInfo);
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/watergun_loop.wav";
		soundUniqueInfo.soundEffectLength = 1.425f;
		soundUniqueInfo.volume = 0.7f;
		playerAudio->defineSoundUnique(Audio::SoundType::SHOOT_LOOP, soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		   Watergun, End		 //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo.playOnce = true;
	soundGeneralInfo.positionalOffset = { 0.0f, 0.0f, 0.0f };
	playerAudio->defineSoundGeneral(Audio::SoundType::SHOOT_END, soundGeneralInfo);
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/watergun_end.wav";
		soundUniqueInfo.soundEffectLength = 0.722f;
		soundUniqueInfo.volume = 0.7f;
		playerAudio->defineSoundUnique(Audio::SoundType::SHOOT_END, soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		   Watergun, Reload		 //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo.playOnce = false;
	soundGeneralInfo.positionalOffset = { 0.0f, 0.0f, 0.0f };
	playerAudio->defineSoundGeneral(Audio::SoundType::RELOAD, soundGeneralInfo);
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/watergun_reload.wav";
		soundUniqueInfo.soundEffectLength = 1.246f;
		soundUniqueInfo.volume = 0.7f;
		playerAudio->defineSoundUnique(Audio::SoundType::RELOAD, soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		   Water Impact, Environment	     //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo.playOnce = true;
	soundGeneralInfo.positionalOffset = { 0.0f, 0.0f, 0.0f };
	playerAudio->defineSoundGeneral(Audio::SoundType::WATER_IMPACT_ENVIRONMENT, soundGeneralInfo);
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/water_drip_1.wav";
		soundUniqueInfo.soundEffectLength = 0.26f;
		soundUniqueInfo.volume = 0.7f;
		playerAudio->defineSoundUnique(Audio::SoundType::WATER_IMPACT_ENVIRONMENT, soundUniqueInfo);
		//	• Sample #2
		soundUniqueInfo.fileName = "../Audio/water_drip_2.wav";
		soundUniqueInfo.soundEffectLength = 0.299f;
		soundUniqueInfo.volume = 0.7f;
		playerAudio->defineSoundUnique(Audio::SoundType::WATER_IMPACT_ENVIRONMENT, soundUniqueInfo);
		//	• Sample #3
		soundUniqueInfo.fileName = "../Audio/water_drip_3.wav";
		soundUniqueInfo.soundEffectLength = 0.26f;
		soundUniqueInfo.volume = 0.7f;
		playerAudio->defineSoundUnique(Audio::SoundType::WATER_IMPACT_ENVIRONMENT, soundUniqueInfo);
		//	• Sample #4
		soundUniqueInfo.fileName = "../Audio/water_drip_4.wav";
		soundUniqueInfo.soundEffectLength = 0.207f;
		soundUniqueInfo.volume = 0.7f;
		playerAudio->defineSoundUnique(Audio::SoundType::WATER_IMPACT_ENVIRONMENT, soundUniqueInfo);
		//	• Sample #5
		soundUniqueInfo.fileName = "../Audio/water_drip_5.wav";
		soundUniqueInfo.soundEffectLength = 0.406f;
		soundUniqueInfo.volume = 0.7f;
		playerAudio->defineSoundUnique(Audio::SoundType::WATER_IMPACT_ENVIRONMENT, soundUniqueInfo);
		//	• Sample #6
		soundUniqueInfo.fileName = "../Audio/water_drip_6.wav";
		soundUniqueInfo.soundEffectLength = 0.463f;
		soundUniqueInfo.volume = 0.7f;
		playerAudio->defineSoundUnique(Audio::SoundType::WATER_IMPACT_ENVIRONMENT, soundUniqueInfo);
		//	• Sample #7
		soundUniqueInfo.fileName = "../Audio/water_drip_7.wav";
		soundUniqueInfo.soundEffectLength = 0.593f;
		soundUniqueInfo.volume = 0.7f;
		playerAudio->defineSoundUnique(Audio::SoundType::WATER_IMPACT_ENVIRONMENT, soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		   Water Impact, Enemy Candle		 //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo.playOnce = true;
	soundGeneralInfo.positionalOffset = { 0.0f, 0.0f, 0.0f };
	playerAudio->defineSoundGeneral(Audio::SoundType::WATER_IMPACT_ENEMY_CANDLE, soundGeneralInfo);
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/water_impact_enemy_candle.wav";
		soundUniqueInfo.soundEffectLength = 0.523f;
		soundUniqueInfo.volume = 0.7f;
		playerAudio->defineSoundUnique(Audio::SoundType::WATER_IMPACT_ENEMY_CANDLE, soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		   Water Impact, My Candle		     //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo.playOnce = true;
	soundGeneralInfo.positionalOffset = { 0.0f, 0.0f, 0.0f };
	playerAudio->defineSoundGeneral(Audio::SoundType::WATER_IMPACT_MY_CANDLE, soundGeneralInfo);
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/water_impact_my_candle.wav";
		soundUniqueInfo.soundEffectLength = 1.364f;
		soundUniqueInfo.volume = 1.0f;
		playerAudio->defineSoundUnique(Audio::SoundType::WATER_IMPACT_MY_CANDLE, soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+//
	//		   Jump	     //   FIN_1.0
	// +-+-+-+-+-+-+-+-+//
	soundGeneralInfo.playOnce = true;
	soundGeneralInfo.positionalOffset = { 0.0f, 0.0f, 0.0f };
	playerAudio->defineSoundGeneral(Audio::SoundType::JUMP, soundGeneralInfo);
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/jump.wav";
		soundUniqueInfo.soundEffectLength = 0.806f;
		soundUniqueInfo.volume = 0.7f;
		playerAudio->defineSoundUnique(Audio::SoundType::JUMP, soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		   Landing, Ground		 //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo.playOnce = false;
	soundGeneralInfo.positionalOffset = { 0.0f, 0.0f, 0.0f };
	playerAudio->defineSoundGeneral(Audio::SoundType::LANDING_GROUND, soundGeneralInfo);
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/landing_ground.wav";
		soundUniqueInfo.soundEffectLength = 0.602f;
		soundUniqueInfo.volume = 0.4f;
		playerAudio->defineSoundUnique(Audio::SoundType::LANDING_GROUND, soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		   Death, Me	     //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo.playOnce = true;
	soundGeneralInfo.positionalOffset = { 0.0f, 0.0f, 0.0f };
	playerAudio->defineSoundGeneral(Audio::SoundType::DEATH_ME, soundGeneralInfo);
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/death_me_1.wav";
		soundUniqueInfo.soundEffectLength = 4.013f;
		soundUniqueInfo.volume = 1.0f;
		playerAudio->defineSoundUnique(Audio::SoundType::DEATH_ME, soundUniqueInfo);
		//	• Sample #2
		soundUniqueInfo.fileName = "../Audio/death_me_2.wav";
		soundUniqueInfo.soundEffectLength = 4.013f;
		soundUniqueInfo.volume = 1.0f;
		playerAudio->defineSoundUnique(Audio::SoundType::DEATH_ME, soundUniqueInfo);
		//	• Sample #3
		soundUniqueInfo.fileName = "../Audio/death_me_3.wav";
		soundUniqueInfo.soundEffectLength = 4.013f;
		soundUniqueInfo.volume = 1.0f;
		playerAudio->defineSoundUnique(Audio::SoundType::DEATH_ME, soundUniqueInfo);
		//	• Sample #4
		soundUniqueInfo.fileName = "../Audio/death_me_4.wav";
		soundUniqueInfo.soundEffectLength = 4.013f;
		soundUniqueInfo.volume = 1.0f;
		playerAudio->defineSoundUnique(Audio::SoundType::DEATH_ME, soundUniqueInfo);
		//	• Sample #5
		soundUniqueInfo.fileName = "../Audio/death_me_5.wav";
		soundUniqueInfo.soundEffectLength = 4.013f;
		soundUniqueInfo.volume = 1.0f;
		playerAudio->defineSoundUnique(Audio::SoundType::DEATH_ME, soundUniqueInfo);
		//	• Sample #6
		soundUniqueInfo.fileName = "../Audio/death_me_6.wav";
		soundUniqueInfo.soundEffectLength = 4.013f;
		soundUniqueInfo.volume = 1.0f;
		playerAudio->defineSoundUnique(Audio::SoundType::DEATH_ME, soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		   Death, Other	     //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo.playOnce = true;
	soundGeneralInfo.positionalOffset = { 0.0f, 0.0f, 0.0f };
	playerAudio->defineSoundGeneral(Audio::SoundType::DEATH_OTHER, soundGeneralInfo);
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/death_enemy.wav";
		soundUniqueInfo.soundEffectLength = 3.578f;
		soundUniqueInfo.volume = 1.0f;
		playerAudio->defineSoundUnique(Audio::SoundType::DEATH_OTHER, soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

#pragma endregion

	// Create candle for the player
	auto e = CreateCandle("PlayerCandle", lightModel, boundingBoxModel, glm::vec3(0.f, 2.f, 0.f), lightIndex);
	e->addComponent<RealTimeComponent>(); // Player candle will have its position updated each frame
	e->getComponent<CandleComponent>()->setOwner(static_cast<int>(playerID));
	player->addChildEntity(e);

	player->getComponent<TransformComponent>()->setStartTranslation(glm::vec3(1.6f, 0.9f, 1.f) + spawnLocation);

	return player;
}

Entity::SPtr EntityFactory::CreateBot(Model* boundingBoxModel, Model* characterModel, const glm::vec3& pos, Model* lightModel, size_t lightIndex, NodeSystem* ns) {

	auto e = ECS::Instance()->createEntity("AiCharacter");
	e->addComponent<ModelComponent>(characterModel);
	e->addComponent<TransformComponent>(pos);
	e->addComponent<BoundingBoxComponent>(boundingBoxModel)->getBoundingBox()->setHalfSize(glm::vec3(0.7f, .9f, 0.7f));
	e->addComponent<CollidableComponent>();
	e->addComponent<MovementComponent>();
	e->addComponent<SpeedLimitComponent>();
	e->addComponent<CollisionComponent>();
	e->addComponent<AiComponent>();
	e->addComponent<CullingComponent>();

	e->addComponent<AudioComponent>();

	e->getComponent<MovementComponent>()->constantAcceleration = glm::vec3(0.0f, -9.8f, 0.0f);
	e->getComponent<SpeedLimitComponent>()->maxSpeed = 3.0f;

	e->addComponent<GunComponent>(nullptr, boundingBoxModel);
	auto aiCandleEntity = EntityFactory::CreateCandle("AiCandle", lightModel, boundingBoxModel, glm::vec3(0.f, 2.f, 0.f), lightIndex);

	e->addChildEntity(aiCandleEntity);
	auto fsmComp = e->addComponent<FSMComponent>();

	// =========Create states and transitions===========

	SearchingState* searchState = fsmComp->createState<SearchingState>(ns);
	AttackingState* attackState = fsmComp->createState<AttackingState>();
	fsmComp->createState<FleeingState>(ns);
	
	// TODO: unnecessary to create new transitions for each FSM if they're all identical
	//Attack State
	FSM::Transition* attackToFleeing = SAIL_NEW FSM::Transition;
	attackToFleeing->addBoolCheck(aiCandleEntity->getComponent<CandleComponent>()->getPtrToIsLit(), false);
	FSM::Transition* attackToSearch = SAIL_NEW FSM::Transition;
	attackToSearch->addFloatGreaterThanCheck(attackState->getDistToHost(), 100.0f);
	
	// Search State
	FSM::Transition* searchToAttack = SAIL_NEW FSM::Transition;
	searchToAttack->addFloatLessThanCheck(searchState->getDistToHost(), 100.0f);
	FSM::Transition* searchToFleeing = SAIL_NEW FSM::Transition;
	searchToFleeing->addBoolCheck(aiCandleEntity->getComponent<CandleComponent>()->getPtrToIsLit(), false);
	
	// Fleeing State
	FSM::Transition* fleeingToSearch = SAIL_NEW FSM::Transition;
	fleeingToSearch->addBoolCheck(aiCandleEntity->getComponent<CandleComponent>()->getPtrToIsLit(), true);
	
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

Entity::SPtr EntityFactory::CreateProjectile(const glm::vec3& pos, const glm::vec3& velocity, bool hasLocalOwner, unsigned __int32 ownersNetId, float lifetime, float randomSpread) {
	auto e = ECS::Instance()->createEntity("projectile");
	glm::vec3 randPos;

	randPos.r = Utils::rnd() * randomSpread;
	randPos.g = Utils::rnd() * randomSpread;
	randPos.b = Utils::rnd() * randomSpread;

	e->addComponent<MetaballComponent>();
	e->addComponent<BoundingBoxComponent>()->getBoundingBox()->setHalfSize(glm::vec3(0.15, 0.15, 0.15));
	e->addComponent<LifeTimeComponent>(lifetime);
	e->addComponent<ProjectileComponent>(10.0f, hasLocalOwner); // TO DO should not be manually set to true
	e->getComponent<ProjectileComponent>()->ownedBy = ownersNetId;
	e->addComponent<TransformComponent>(pos + randPos);
	if (hasLocalOwner == true) {
		e->addComponent<LocalOwnerComponent>(ownersNetId);
	}
	else {
		e->addComponent<OnlineOwnerComponent>(ownersNetId);
	}
	

	MovementComponent* movement = e->addComponent<MovementComponent>();
	movement->velocity = velocity;
	movement->constantAcceleration = glm::vec3(0.f, -9.8f, 0.f);

	CollisionComponent* collision = e->addComponent<CollisionComponent>();
	collision->drag = 2.0f;
	// NOTE: 0.0f <= Bounciness <= 1.0f
	collision->bounciness = 0.1f;
	collision->padding = 0.2f;

	return e;
}
