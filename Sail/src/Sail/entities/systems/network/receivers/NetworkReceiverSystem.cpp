#include "pch.h"
#include "NetworkReceiverSystem.h"
#include "Sail/entities/Entity.h"
#include "Sail/entities/components/NetworkReceiverComponent.h"
#include "Sail/entities/systems/network/NetworkSenderSystem.h"
#include "Sail/entities/components/OnlineOwnerComponent.h"
#include "Sail/entities/components/LocalOwnerComponent.h"
#include "Sail/entities/systems/Gameplay/LevelSystem/LevelSystem.h"
#include "Sail/entities/systems/Gameplay/SprinklerSystem.h"

#include "Network/NWrapperSingleton.h"
#include "Sail/netcode/ArchiveTypes.h"
#include "../Sail/src/Sail/events/types/LoopShootingEvent.h"

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


// TODO: register more components
NetworkReceiverSystem::NetworkReceiverSystem() : ReceiverBase() {
	registerComponent<NetworkReceiverComponent>(true, true, true);
	registerComponent<TransformComponent>(false, true, true);

	EventDispatcher::Instance().subscribe(Event::Type::NETWORK_DISCONNECT, this);
}

NetworkReceiverSystem::~NetworkReceiverSystem() {
	EventDispatcher::Instance().unsubscribe(Event::Type::NETWORK_DISCONNECT, this);
}


void NetworkReceiverSystem::pushDataToBuffer(const std::string& data) {
	std::lock_guard<std::mutex> lock(m_bufferLock);
	m_incomingDataBuffer.push(data);
}


void NetworkReceiverSystem::update(float dt) {
	std::lock_guard<std::mutex> lock(m_bufferLock);

	processData(dt, m_incomingDataBuffer);
}

void NetworkReceiverSystem::createPlayer(const PlayerComponentInfo& info, const glm::vec3& pos) {
	// Early exit if the entity already exists
	if (findFromNetID(info.playerCompID)) {
		return;
	}

	auto e = ECS::Instance()->createEntity("networkedEntity");
	instantAddEntity(e.get());

	SAIL_LOG("Created player with id: " + std::to_string(info.playerCompID));

	// lightIndex set to 999, can probably be removed since it no longer seems to be used
	EntityFactory::CreateOtherPlayer(e, info.playerCompID, info.candleID, info.gunID, 999, pos);
	ECS::Instance()->addAllQueuedEntities();
}

void NetworkReceiverSystem::destroyEntity(const Netcode::ComponentID entityID) {
	if (auto e = findFromNetID(entityID); e) {
		e->queueDestruction();
		return;
	}
	SAIL_LOG_WARNING("destoryEntity called but no matching entity found");
}

void NetworkReceiverSystem::enableSprinklers() {
	ECS::Instance()->getSystem<SprinklerSystem>()->enableSprinklers();
}

void NetworkReceiverSystem::extinguishCandle(const Netcode::ComponentID candleId, const Netcode::PlayerID shooterID) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == candleId) {

			e->getComponent<CandleComponent>()->wasJustExtinguished = true;
			e->getComponent<CandleComponent>()->wasHitByPlayerID = shooterID;

			return;
		}
	}
	SAIL_LOG_WARNING("extinguishCandle called but no matching candle entity found");
}

void NetworkReceiverSystem::hitBySprinkler(const Netcode::ComponentID candleOwnerID) {
	waterHitPlayer(candleOwnerID, Netcode::MESSAGE_SPRINKLER_ID);
}

void NetworkReceiverSystem::igniteCandle(const Netcode::ComponentID candleID) {
	EventDispatcher::Instance().emit(IgniteCandleEvent(candleID));
}

void NetworkReceiverSystem::playerDied(const Netcode::ComponentID networkIdOfKilled, const Netcode::PlayerID playerIdOfShooter) {
	if (auto e = findFromNetID(networkIdOfKilled); e) {
		EventDispatcher::Instance().emit(PlayerDiedEvent(
			e,
			m_playerEntity,
			playerIdOfShooter,
			networkIdOfKilled)
		);
		return;
	}
	SAIL_LOG_WARNING("playerDied called but no matching entity found");
}

void NetworkReceiverSystem::setAnimation(const Netcode::ComponentID id, const AnimationInfo& info) {
	if (auto e = findFromNetID(id); e) {
		auto animation = e->getComponent<AnimationComponent>();
		animation->setAnimation(info.index);
		animation->animationTime = info.time;
		return;
	}
	SAIL_LOG_WARNING("setAnimation called but no matching entity found");
}

void NetworkReceiverSystem::setCandleHealth(const Netcode::ComponentID candleId, const float health) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == candleId) {
			e->getComponent<CandleComponent>()->health = health;
			return;
		}
	}
	SAIL_LOG_WARNING("setCandleHelath called but no matching candle entity found");
}

// The player who puts down their candle does this in CandleSystem and tests collisions
// The candle will be moved for everyone else in here
void NetworkReceiverSystem::setCandleState(const Netcode::ComponentID id, const bool isHeld) {
	EventDispatcher::Instance().emit(HoldingCandleToggleEvent(id, isHeld));
}

// Might need some optimization (like sorting) if we have a lot of networked entities
void NetworkReceiverSystem::setLocalPosition(const Netcode::ComponentID id, const glm::vec3& translation) {
	if (auto e = findFromNetID(id); e) {
		e->getComponent<TransformComponent>()->setTranslation(translation);
		return;
	}
	SAIL_LOG_WARNING("setLocalPosition called but no matching entity found");
}

void NetworkReceiverSystem::setLocalRotation(const Netcode::ComponentID id, const glm::vec3& rotation) {
	if (auto e = findFromNetID(id); e) {
		e->getComponent<TransformComponent>()->setRotations(rotation);
		return;
	}
	SAIL_LOG_WARNING("setLocalRotation called but no matching entity found");
}

void NetworkReceiverSystem::setLocalRotation(const Netcode::ComponentID id, const glm::quat& rotation) {
	if (auto e = findFromNetID(id); e) {
		e->getComponent<TransformComponent>()->setRotations(rotation);
		return;
	}
	SAIL_LOG_WARNING("setLocalRotation called but no matching entity found");
}

// If I requested the projectile it has a local owner
void NetworkReceiverSystem::spawnProjectile(const ProjectileInfo& info) {
	const bool wasRequestedByMe = (Netcode::getComponentOwner(info.ownerID) == m_playerID);

	auto e = ECS::Instance()->createEntity("projectile");
	instantAddEntity(e.get());

	EntityFactory::ProjectileArguments args{};
	args.pos           = info.position;
	args.velocity      = info.velocity;
	args.hasLocalOwner = wasRequestedByMe;
	args.ownersNetId   = info.ownerID;
	args.netCompId     = info.projectileID;

	EntityFactory::CreateProjectile(e, args);
}

void NetworkReceiverSystem::waterHitPlayer(const Netcode::ComponentID id, const Netcode::PlayerID senderId) {
	EventDispatcher::Instance().emit(WaterHitPlayerEvent(id, senderId));
}



// AUDIO

void NetworkReceiverSystem::playerJumped(const Netcode::ComponentID id) {
	EventDispatcher::Instance().emit(PlayerJumpedEvent(id));
}

void NetworkReceiverSystem::playerLanded(const Netcode::ComponentID id) {
	EventDispatcher::Instance().emit(PlayerLandedEvent(id));
}


// TODO: Remove info since it's unused or are these functions not finished?
void NetworkReceiverSystem::shootStart(const Netcode::ComponentID id, float frequency) {
	// Only called when another player shoots
	EventDispatcher::Instance().emit(StartShootingEvent(id, frequency));
}

void NetworkReceiverSystem::shootLoop(const Netcode::ComponentID id, float frequency) {
	// Only called when another player shoots
	EventDispatcher::Instance().emit(LoopShootingEvent(id, frequency));
}

void NetworkReceiverSystem::shootEnd(const Netcode::ComponentID id, float frequency) {
	// Only called when another player shoots
	EventDispatcher::Instance().emit(StopShootingEvent(id, frequency));
}

void NetworkReceiverSystem::runningMetalStart(const Netcode::ComponentID id) {
	EventDispatcher::Instance().emit(ChangeWalkingSoundEvent(id, Audio::SoundType::RUN_METAL));
}

void NetworkReceiverSystem::runningTileStart(const Netcode::ComponentID id) {
	EventDispatcher::Instance().emit(ChangeWalkingSoundEvent(id, Audio::SoundType::RUN_TILE));
}
void NetworkReceiverSystem::runningWaterMetalStart(Netcode::ComponentID id) {
	EventDispatcher::Instance().emit(ChangeWalkingSoundEvent(id, Audio::SoundType::RUN_WATER_METAL));
}

void NetworkReceiverSystem::runningWaterTileStart(Netcode::ComponentID id) {
	EventDispatcher::Instance().emit(ChangeWalkingSoundEvent(id, Audio::SoundType::RUN_WATER_TILE));
}

void NetworkReceiverSystem::runningStopSound(const Netcode::ComponentID id) {
	EventDispatcher::Instance().emit(StopWalkingEvent(id));
}

void NetworkReceiverSystem::throwingStartSound(const Netcode::ComponentID id) {
	EventDispatcher::Instance().emit(StartThrowingEvent(id));
}

void NetworkReceiverSystem::throwingEndSound(const Netcode::ComponentID id) {
	EventDispatcher::Instance().emit(StopThrowingEvent(id));
}

// NOT FROM SERIALIZED MESSAGES

// TODO: Save a message to KillCamReceiverSystem so that disconnected players disappear at the correct time in the replay
// NOTE: This is not called on the host, since the host receives the disconnect through NWrapperHost::playerDisconnected()
void NetworkReceiverSystem::playerDisconnect(const Netcode::PlayerID playerID) {
	for (auto e : entities) {
		// Find the player entity and delete it
		if (e->hasComponent<PlayerComponent>() && Netcode::getComponentOwner(e->getComponent<NetworkReceiverComponent>()->m_id) == playerID) {
			e->removeDeleteAllChildren();
			e->queueDestruction();
			return;
		}
	}
	SAIL_LOG_WARNING("playerDisconnect called but no matching entity found");
}



// Helper function
Entity* NetworkReceiverSystem::findFromNetID(const Netcode::ComponentID id) const {
	for (auto e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == id) {
			return e;
		}
	}
	return nullptr;
}


bool NetworkReceiverSystem::onEvent(const Event& event) {
	switch (event.type) {
	case Event::Type::NETWORK_DISCONNECT:
		playerDisconnect(((const NetworkDisconnectEvent&)(event)).player.id);
		break;
	default: 
		break;
	}

	return true;
}