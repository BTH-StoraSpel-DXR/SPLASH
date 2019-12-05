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

void NetworkReceiverSystem::stop() {
	m_incomingDataBuffer = std::queue<std::string>(); // Clear the data buffer
	m_netSendSysPtr = nullptr;
}


void NetworkReceiverSystem::init(Netcode::PlayerID player, NetworkSenderSystem* NSS) {
	initBase(player);
	m_netSendSysPtr = NSS;
}

void NetworkReceiverSystem::pushDataToBuffer(const std::string& data) {
	std::lock_guard<std::mutex> lock(m_bufferLock);
	m_incomingDataBuffer.push(data);
}


void NetworkReceiverSystem::update(float dt) {
	std::lock_guard<std::mutex> lock(m_bufferLock);

	processData(dt, m_incomingDataBuffer);
}

#ifdef DEVELOPMENT
unsigned int NetworkReceiverSystem::getByteSize() const {
	unsigned int size = BaseComponentSystem::getByteSize() + sizeof(*this);
	const size_t queueSize = m_incomingDataBuffer.size();
	size += queueSize * sizeof(std::string);
	if (queueSize) {
		size += queueSize * m_incomingDataBuffer.front().capacity() * sizeof(unsigned char);	// Approximate string length
	}
	return 0;
}
#endif

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

void NetworkReceiverSystem::endMatch(const GameDataForOthersInfo& info) {

	GameDataTracker::getInstance().setStatsForOtherData(
		info.bulletsFiredID, info.bulletsFired,
		info.distanceWalkedID, info.distanceWalked,
		info.jumpsMadeID, info.jumpsMade);

	endGame(); // This function does different things depending on if you are the host or the client
}

void NetworkReceiverSystem::extinguishCandle(const Netcode::ComponentID candleId, const Netcode::PlayerID shooterID) {
	for (auto& e : entities) {
		if (e->getComponent<NetworkReceiverComponent>()->m_id == candleId) {
			e->getComponent<CandleComponent>()->wasJustExtinguished = true;
			e->getComponent<CandleComponent>()->wasHitByPlayerID = shooterID;

			EventDispatcher::Instance().emit(TorchExtinguishedEvent(shooterID, candleId));

			return;
		}
	}
	SAIL_LOG_WARNING("extinguishCandle called but no matching candle entity found");
}

void NetworkReceiverSystem::hitBySprinkler(const Netcode::ComponentID candleOwnerID) {
	if (auto e = findFromNetID(candleOwnerID); e) {
		waterHitPlayer(candleOwnerID, Netcode::SPRINKLER_COMP_ID);
		return;
	}
	SAIL_LOG_WARNING("hitBySprinkler called but no matching entity found");
}

void NetworkReceiverSystem::igniteCandle(const Netcode::ComponentID candleID) {
	if (auto e = findFromNetID(candleID); e) {
		EventDispatcher::Instance().emit(IgniteCandleEvent(candleID));
		return;
	}
	SAIL_LOG_WARNING("igniteCandle called but no matching entity found");
}

void NetworkReceiverSystem::matchEnded() {

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

void NetworkReceiverSystem::playerDied(const Netcode::ComponentID networkIdOfKilled, const KillInfo& info) {
	if (auto e = findFromNetID(networkIdOfKilled); e) {
		EventDispatcher::Instance().emit(PlayerDiedEvent(
			e,
			m_playerEntity,
			info.killerCompID,
			networkIdOfKilled,
			info.isFinal)
		);
		return;
	}
	SAIL_LOG_WARNING("playerDied called but no matching entity found");
}

void NetworkReceiverSystem::setAnimation(const Netcode::ComponentID id, const AnimationInfo& info) {
	if (auto e = findFromNetID(id); e) {
		auto animation = e->getComponent<AnimationComponent>();
		animation->setAnimation(info.index, false);
		animation->animationTime = info.time;
		animation->pitch = info.pitch;
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
	if (auto e = findFromNetID(id); e) {
		EventDispatcher::Instance().emit(HoldingCandleToggleEvent(id, isHeld));
		return;
	}
	SAIL_LOG_WARNING("setCandleState called but no matching entity found");
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

void NetworkReceiverSystem::setPlayerStats(const PlayerStatsInfo& info) {
	GameDataTracker::getInstance().setStatsForPlayer(info.player, info.nrOfKills, info.placement, info.nDeaths, info.damage, info.damageTaken);
}

void NetworkReceiverSystem::updateSanity(const Netcode::ComponentID id, const float sanity) {
	if (auto e = findFromNetID(id); e) {
		EventDispatcher::Instance().emit(UpdateSanityEvent(id, sanity));
		return;
	}
	SAIL_LOG_WARNING("updateSanity called but no matching entity found");
}

void NetworkReceiverSystem::updateProjectile(const Netcode::ComponentID id, const glm::vec3& pos, const glm::vec3& vel) {
	if (auto e = findFromNetID(id); e) {
		e->getComponent<TransformComponent>()->setTranslation(pos);
		e->getComponent<MovementComponent>()->velocity = vel;

		Application::getInstance()->getRenderWrapper()->getCurrentRenderer()->submitWaterPoint(pos);
		return;
	}
	SAIL_LOG_WARNING("updateProjectile called but no matching entity found");
}

// If I requested the projectile it has a local owner
void NetworkReceiverSystem::spawnProjectile(const ProjectileInfo& info) {
	const bool wasRequestedByMe = (Netcode::getComponentOwner(info.ownerID) == m_playerID);

	auto e = ECS::Instance()->createEntity("projectile");
	instantAddEntity(e.get());

	EntityFactory::ProjectileArguments args{};
	args.pos            = info.position;
	args.velocity       = info.velocity;
	args.hasLocalOwner  = wasRequestedByMe;
	args.ownersNetId    = info.ownerID;
	args.netCompId      = info.projectileID;

#ifdef _PERFORMANCE_TEST
	if (!wasRequestedByMe) {
		// Since the player who spawns a projectile is responsible for destroying it and the projectiles in the 
		// performance test don't have owners, we limit their lifetime to make it more comparable with how many
		// projectiles there would be in the world if you had 12 actual players firing simultaneously.
		args.lifetime = 0.4f;
	}
#endif

	EntityFactory::CreateProjectile(e, args);
}

void NetworkReceiverSystem::submitWaterPoint(const glm::vec3& point) {
	// Place water point at intersection position
	Application::getInstance()->getRenderWrapper()->getCurrentRenderer()->submitWaterPoint(point);
}


// TODO only emit events if the entities still exist
void NetworkReceiverSystem::waterHitPlayer(const Netcode::ComponentID id, const Netcode::ComponentID projectileID) {
	if (auto e = findFromNetID(id); e) {
		EventDispatcher::Instance().emit(WaterHitPlayerEvent(id, projectileID));
		return;
	}
	SAIL_LOG_WARNING("waterHitPLayer called but no matching entity found");
}



// AUDIO

void NetworkReceiverSystem::playerJumped(const Netcode::ComponentID id) {
	if (auto e = findFromNetID(id); e) {
		EventDispatcher::Instance().emit(PlayerJumpedEvent(id));
		return;
	}
	SAIL_LOG_WARNING("waterHitPLayer called but no matching entity found");
}

void NetworkReceiverSystem::playerLanded(const Netcode::ComponentID id) {
	if (auto e = findFromNetID(id); e) {
		EventDispatcher::Instance().emit(PlayerLandedEvent(id));
		return;
	}
	SAIL_LOG_WARNING("playerLanded called but no matching entity found");
}


void NetworkReceiverSystem::shootStart(const Netcode::ComponentID id, float frequency) {
	// Only called when another player shoots
	if (auto e = findFromNetID(id); e) {
		EventDispatcher::Instance().emit(StartShootingEvent(id, frequency));
		return;
	}
	SAIL_LOG_WARNING("shootStart called but no matching entity found");
}

void NetworkReceiverSystem::shootLoop(const Netcode::ComponentID id, float frequency) {
	// Only called when another player shoots
	if (auto e = findFromNetID(id); e) {
		EventDispatcher::Instance().emit(LoopShootingEvent(id, frequency));
		return;
	}
	SAIL_LOG_WARNING("shootLoop called but no matching entity found");
}

void NetworkReceiverSystem::shootEnd(const Netcode::ComponentID id, float frequency) {
	// Only called when another player shoots
	if (auto e = findFromNetID(id); e) {
		EventDispatcher::Instance().emit(StopShootingEvent(id, frequency));
		return;
	}
	SAIL_LOG_WARNING("shootEnd called but no matching entity found");
}

void NetworkReceiverSystem::runningMetalStart(const Netcode::ComponentID id) {
	if (auto e = findFromNetID(id); e) {
		EventDispatcher::Instance().emit(ChangeWalkingSoundEvent(id, Audio::SoundType::RUN_METAL));
		return;
	}
	SAIL_LOG_WARNING("runningMetalStart called but no matching entity found");
}

void NetworkReceiverSystem::runningTileStart(const Netcode::ComponentID id) {
	if (auto e = findFromNetID(id); e) {
		EventDispatcher::Instance().emit(ChangeWalkingSoundEvent(id, Audio::SoundType::RUN_TILE));
		return;
	}
	SAIL_LOG_WARNING("runningTileStart called but no matching entity found");
}

void NetworkReceiverSystem::runningWaterMetalStart(Netcode::ComponentID id) {
	if (auto e = findFromNetID(id); e) {
		EventDispatcher::Instance().emit(ChangeWalkingSoundEvent(id, Audio::SoundType::RUN_WATER_METAL));
		return;
	}
	SAIL_LOG_WARNING("runningWaterMetalStart called but no matching entity found");
}

void NetworkReceiverSystem::runningWaterTileStart(Netcode::ComponentID id) {
	if (auto e = findFromNetID(id); e) {
		EventDispatcher::Instance().emit(ChangeWalkingSoundEvent(id, Audio::SoundType::RUN_WATER_TILE));
		return;
	}
	SAIL_LOG_WARNING("runningWaterTileStart called but no matching entity found");
}

void NetworkReceiverSystem::runningStopSound(const Netcode::ComponentID id) {
	if (auto e = findFromNetID(id); e) {
		EventDispatcher::Instance().emit(StopWalkingEvent(id));
		return;
	}
	SAIL_LOG_WARNING("runningStopSound called but no matching entity found");
}

void NetworkReceiverSystem::throwingStartSound(const Netcode::ComponentID id) {
	if (auto e = findFromNetID(id); e) {
		EventDispatcher::Instance().emit(StartThrowingEvent(id));
		return;
	}
	SAIL_LOG_WARNING("throwingStartSound called but no matching entity found");
}

void NetworkReceiverSystem::throwingEndSound(const Netcode::ComponentID id) {
	if (auto e = findFromNetID(id); e) {
		EventDispatcher::Instance().emit(StopThrowingEvent(id));
		return;
	}
	SAIL_LOG_WARNING("throwingEndSound called but no matching entity found");
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