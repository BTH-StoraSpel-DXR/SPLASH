#include "pch.h"
#include "KillCamReceiverSystem.h"
#include "Sail/entities/Entity.h"

// TODO: Remove unnecessary includes

#include "Sail/entities/components/NetworkReceiverComponent.h"
#include "Sail/entities/systems/network/NetworkSenderSystem.h"
#include "Sail/entities/components/OnlineOwnerComponent.h"
#include "Sail/entities/components/LocalOwnerComponent.h"
#include "Sail/entities/systems/Gameplay/LevelSystem/LevelSystem.h"
#include "Sail/entities/systems/Gameplay/SprinklerSystem.h"

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
static std::ofstream out("LogFiles/KillCamReceiverSystem.cpp.log");
#endif


// TODO: register more components
KillCamReceiverSystem::KillCamReceiverSystem() : ReceiverBase() {
	registerComponent<ReplayComponent>(true, false, false);
	registerComponent<ReplayTransformComponent>(true, true, true);

	//EventDispatcher::Instance().subscribe(Event::Type::NETWORK_DISCONNECT, this);
}

KillCamReceiverSystem::~KillCamReceiverSystem() {
	//EventDispatcher::Instance().unsubscribe(Event::Type::NETWORK_DISCONNECT, this);
}

void KillCamReceiverSystem::handleIncomingData(const std::string& data) {
	std::lock_guard<std::mutex> lock(m_replayDataLock);

	m_replayData[m_currentWriteInd].push(data);
}


// Prepare transform components for the next frame
void KillCamReceiverSystem::prepareUpdate() {
	for (auto e : entities) {

		e->getComponent<ReplayTransformComponent>()->prepareUpdate();
	}
}

// Increments the indexes in the ring buffer once per tick and clears the next write-index
void KillCamReceiverSystem::update(float dt) {
	std::lock_guard<std::mutex> lock(m_replayDataLock);

	m_currentWriteInd = ++m_currentWriteInd % REPLAY_BUFFER_SIZE;
	m_currentReadInd  = ++m_currentReadInd  % REPLAY_BUFFER_SIZE;

	m_replayData[m_currentWriteInd] = std::queue<std::string>(); // Clear the current write-position's queue in the ring buffer
}

// Should only be called when the killcam is active
void KillCamReceiverSystem::processReplayData(float dt) {
	std::lock_guard<std::mutex> lock(m_replayDataLock);

	processData(dt, m_replayData[m_currentReadInd], false);
}


void KillCamReceiverSystem::createPlayer(const PlayerComponentInfo& info, const glm::vec3& pos) {
	//// Early exit if the entity already exists
	//if (findFromNetID(info.playerCompID)) {
	//	return;
	//}

	//auto e = ECS::Instance()->createEntity("networkedEntity");
	//instantAddEntity(e.get());

	//SAIL_LOG("Created player with id: " + std::to_string(info.playerCompID));

	//// lightIndex set to 999, can probably be removed since it no longer seems to be used
	//EntityFactory::CreateOtherPlayer(e, info.playerCompID, info.candleID, info.gunID, 999, pos);

}

void KillCamReceiverSystem::destroyEntity(const Netcode::ComponentID entityID) {
	if (auto e = findFromNetID(entityID); e) {
		e->queueDestruction();
		return;
	}
	SAIL_LOG_WARNING("destoryEntity called but no matching entity found");
}

void KillCamReceiverSystem::enableSprinklers() {
	//ECS::Instance()->getSystem<SprinklerSystem>()->enableSprinklers();
}

void KillCamReceiverSystem::extinguishCandle(const Netcode::ComponentID candleId, const Netcode::PlayerID shooterID) {
	//for (auto& e : entities) {
	//	if (e->getComponent<NetworkReceiverComponent>()->m_id == candleId) {

	//		e->getComponent<CandleComponent>()->wasJustExtinguished = true;
	//		e->getComponent<CandleComponent>()->wasHitByPlayerID = shooterID;

	//		return;
	//	}
	//}
	//SAIL_LOG_WARNING("extinguishCandle called but no matching candle entity found");
}

void KillCamReceiverSystem::hitBySprinkler(const Netcode::ComponentID candleOwnerID) {
	//waterHitPlayer(candleOwnerID, Netcode::MESSAGE_SPRINKLER_ID);
}

void KillCamReceiverSystem::igniteCandle(const Netcode::ComponentID candleID) {
	//EventDispatcher::Instance().emit(IgniteCandleEvent(candleID));
}

void KillCamReceiverSystem::playerDied(const Netcode::ComponentID networkIdOfKilled, const Netcode::PlayerID playerIdOfShooter) {
	//if (auto e = findFromNetID(networkIdOfKilled); e) {
	//	EventDispatcher::Instance().emit(PlayerDiedEvent(
	//		e,
	//		m_playerEntity,
	//		playerIdOfShooter,
	//		networkIdOfKilled)
	//	);
	//	return;
	//}
	//SAIL_LOG_WARNING("playerDied called but no matching entity found");
}

void KillCamReceiverSystem::setAnimation(const Netcode::ComponentID id, const AnimationInfo& info) {
	//if (auto e = findFromNetID(id); e) {
	//	auto animation = e->getComponent<AnimationComponent>();
	//	animation->setAnimation(info.index);
	//	animation->animationTime = info.time;
	//	return;
	//}
	//SAIL_LOG_WARNING("setAnimation called but no matching entity found");
}

void KillCamReceiverSystem::setCandleHealth(const Netcode::ComponentID candleId, const float health) {
	//for (auto& e : entities) {
	//	if (e->getComponent<NetworkReceiverComponent>()->m_id == candleId) {
	//		e->getComponent<CandleComponent>()->health = health;
	//		return;
	//	}
	//}
	//SAIL_LOG_WARNING("setCandleHelath called but no matching candle entity found");
}

// The player who puts down their candle does this in CandleSystem and tests collisions
// The candle will be moved for everyone else in here
void KillCamReceiverSystem::setCandleState(const Netcode::ComponentID id, const bool isHeld) {

	//EventDispatcher::Instance().emit(HoldingCandleToggleEvent(id, isHeld));

}

// Might need some optimization (like sorting) if we have a lot of networked entities
void KillCamReceiverSystem::setLocalPosition(const Netcode::ComponentID id, const glm::vec3& translation) {
	if (auto e = findFromNetID(id); e) {
		e->getComponent<ReplayTransformComponent>()->setTranslation(translation);
		return;
	}
	SAIL_LOG_WARNING("setLocalPosition called but no matching entity found");
}

void KillCamReceiverSystem::setLocalRotation(const Netcode::ComponentID id, const glm::vec3& rotation) {
	if (auto e = findFromNetID(id); e) {
		e->getComponent<ReplayTransformComponent>()->setRotations(rotation);
		return;
	}
	SAIL_LOG_WARNING("setLocalRotation called but no matching entity found");
}

void KillCamReceiverSystem::setLocalRotation(const Netcode::ComponentID id, const glm::quat& rotation) {
	if (auto e = findFromNetID(id); e) {
		e->getComponent<ReplayTransformComponent>()->setRotations(rotation);
		return;
	}
	SAIL_LOG_WARNING("setLocalRotation called but no matching entity found");
}

// If I requested the projectile it has a local owner
void KillCamReceiverSystem::spawnProjectile(const ProjectileInfo& info) {
	auto e = ECS::Instance()->createEntity("projectile");
	instantAddEntity(e.get());

	EntityFactory::ProjectileArguments args{};
	args.pos = info.position;
	args.velocity = info.velocity;
	args.ownersNetId = info.ownerID;
	args.netCompId = info.projectileID;

	EntityFactory::CreateReplayProjectile(e, args);
}

void KillCamReceiverSystem::waterHitPlayer(const Netcode::ComponentID id, const Netcode::PlayerID senderId) {
	//EventDispatcher::Instance().emit(WaterHitPlayerEvent(id, senderId));
}



// AUDIO

void KillCamReceiverSystem::playerJumped(const Netcode::ComponentID id) {
	//EventDispatcher::Instance().emit(PlayerJumpedEvent(id));
}

void KillCamReceiverSystem::playerLanded(const Netcode::ComponentID id) {
	//EventDispatcher::Instance().emit(PlayerLandedEvent(id));
}


// TODO: Remove info since it's unused or are these functions not finished?
void KillCamReceiverSystem::shootStart(const Netcode::ComponentID id, float frequency) {
	// Only called when another player shoots
	//EventDispatcher::Instance().emit(StartShootingEvent(id));
}

void KillCamReceiverSystem::shootLoop(const Netcode::ComponentID id, float frequency) {
	// Only called when another player shoots
	//if (auto e = findFromNetID(id); e) {
	//	e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_START].isPlaying = false;
	//	e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_LOOP].isPlaying = true;
	//	e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_LOOP].playOnce = true;
	//	return;
	//}
	//SAIL_LOG_WARNING("shootLoop called but no matching entity found");
}

void KillCamReceiverSystem::shootEnd(const Netcode::ComponentID id, float frequency) {
	// Only called when another player shoots
	//EventDispatcher::Instance().emit(StopShootingEvent(id));
}

void KillCamReceiverSystem::runningMetalStart(const Netcode::ComponentID id) {
	//EventDispatcher::Instance().emit(ChangeWalkingSoundEvent(id, Audio::SoundType::RUN_METAL));
}

void KillCamReceiverSystem::runningWaterMetalStart(const Netcode::ComponentID id) {
	//EventDispatcher::Instance().emit(ChangeWalkingSoundEvent(id, Audio::SoundType::RUN_METAL));
}

void KillCamReceiverSystem::runningTileStart(const Netcode::ComponentID id) {
	//EventDispatcher::Instance().emit(ChangeWalkingSoundEvent(id, Audio::SoundType::RUN_TILE));
}

void KillCamReceiverSystem::runningWaterTileStart(const Netcode::ComponentID id) {
	//EventDispatcher::Instance().emit(ChangeWalkingSoundEvent(id, Audio::SoundType::RUN_TILE));
}

void KillCamReceiverSystem::runningStopSound(const Netcode::ComponentID id) {
	//EventDispatcher::Instance().emit(StopWalkingEvent(id));
}

void KillCamReceiverSystem::throwingStartSound(const Netcode::ComponentID id) {
	//EventDispatcher::Instance().emit(StartThrowingEvent(id));
}

void KillCamReceiverSystem::throwingEndSound(const Netcode::ComponentID id) {
	//EventDispatcher::Instance().emit(StopThrowingEvent(id));
}



// These functions are only used by NetworkReceiverSystemHost so their implementations are empty here
void KillCamReceiverSystem::endMatch() {}
void KillCamReceiverSystem::endMatchAfterTimer(const float dt) {}
void KillCamReceiverSystem::prepareEndScreen(const Netcode::PlayerID sender, const EndScreenInfo& info) {}
void KillCamReceiverSystem::mergeHostsStats() {}



// NOT FROM SERIALIZED MESSAGES

// NOTE: This is not called on the host, since the host receives the disconnect through NWrapperHost::playerDisconnected()
void KillCamReceiverSystem::playerDisconnect(const Netcode::PlayerID playerID) 
{}


// Helper function

Entity* KillCamReceiverSystem::findFromNetID(const Netcode::ComponentID id) const {
	for (auto e : entities) {
		if (e->getComponent<ReplayComponent>()->m_id == id) {
			return e;
		}
	}
	return nullptr;
}


bool KillCamReceiverSystem::onEvent(const Event& event) {
	switch (event.type) {
	default: 
		break;
	}

	return true;
}