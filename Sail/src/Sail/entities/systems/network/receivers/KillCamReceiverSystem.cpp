#include "pch.h"
#include "KillCamReceiverSystem.h"
#include "Sail/entities/Entity.h"

// TODO: Remove unnecessary includes

#include "Sail/entities/components/components.h"
#include "Sail/entities/systems/network/NetworkSenderSystem.h"
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

#include "Sail/graphics/camera/CameraController.h"


//#define _LOG_TO_FILE
#if defined(DEVELOPMENT) && defined(_LOG_TO_FILE)
#include <fstream>
static std::ofstream out("LogFiles/KillCamReceiverSystem.cpp.log");
#endif


// TODO: register more components
KillCamReceiverSystem::KillCamReceiverSystem() : ReceiverBase() {
	registerComponent<ReplayReceiverComponent>(true, false, false);

	EventDispatcher::Instance().subscribe(Event::Type::TOGGLE_SLOW_MOTION, this);
	EventDispatcher::Instance().subscribe(Event::Type::TORCH_NOT_HELD, this);
	EventDispatcher::Instance().subscribe(Event::Type::START_KILLCAM, this);

}

KillCamReceiverSystem::~KillCamReceiverSystem() {
	EventDispatcher::Instance().unsubscribe(Event::Type::TOGGLE_SLOW_MOTION, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::TORCH_NOT_HELD, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::START_KILLCAM, this);

	stop();
}


float KillCamReceiverSystem::getKillCamAlpha(const float alpha) const {
	if (m_slowMotionState == SlowMotionSetting::ENABLE) {
		return (static_cast<float>(m_killCamTickCounter) + alpha) / static_cast<float>(SLOW_MO_MULTIPLIER);
	} else {
		return alpha;
	}
}

float KillCamReceiverSystem::getKillCamDelta(const float delta) const {
	return (m_slowMotionState == SlowMotionSetting::ENABLE) ? (delta / SLOW_MO_MULTIPLIER) : delta;
}

// If slow motion is enabled only update once every SLOW_MO_MULTIPLIER ticks
bool KillCamReceiverSystem::skipUpdate() {
	m_killCamTickCounter = (++m_killCamTickCounter) % SLOW_MO_MULTIPLIER;
	return (m_slowMotionState == SlowMotionSetting::ENABLE && m_killCamTickCounter != 0);
}


void KillCamReceiverSystem::stop() {
	// Clear the saved data
	for (std::vector<Netcode::ComponentID>& data : m_notHoldingTorches) {
		data.clear();
	}
	m_replayData.clear();
	m_myKillCamData.clear();

	m_slowMotionState = SlowMotionSetting::DISABLE;
	m_hasStarted      = false;
	m_isFinalKillCam    = false;
	m_killCamTickCounter = 0;
	m_idOfKillingProjectile = Netcode::UNINITIALIZED;
	m_killerPlayer       = nullptr;
	m_killerProjectile   = nullptr;
	m_trackingProjectile = false;
	Memory::SafeDelete(m_cam);

	m_projectilePos = { 0,0,0 };
	m_killerHeadPos = { 0,0,0 };
}

bool KillCamReceiverSystem::startKillCam() {
	const Netcode::PlayerID killerID = Netcode::getComponentOwner(m_idOfKillingProjectile);

	// Copy the past few seconds to data that we'll read from for our own killcam
	m_myKillCamData = m_replayData;

	for (auto e : entities) {
		const Netcode::ComponentID compID = e->getComponent<ReplayReceiverComponent>()->m_id;

		e->tryToAddToSystems = true;
		e->addComponent<RenderInReplayComponent>();


		if (e->hasComponent<PlayerComponent>()) {
			if (killerID == Netcode::getComponentOwner(compID)) {
				e->addComponent<KillerComponent>();
				m_killerPlayer = e;
			}

			// Put torches where they were at that point in time
			for (Netcode::ComponentID& notHolding : m_notHoldingTorches[m_myKillCamData.readIndex]) {
				if (compID == notHolding) {
					setCandleState(compID, false);
				}
			}
		}
	}
	m_hasStarted = true;

	return true;
}

void KillCamReceiverSystem::stopMyKillCam() {
	m_slowMotionState = SlowMotionSetting::DISABLE;
	m_hasStarted = false;
	m_idOfKillingProjectile = Netcode::UNINITIALIZED;
	m_killerPlayer = nullptr;
	m_killerProjectile = nullptr;
	m_trackingProjectile = false;

	m_projectilePos = { 0,0,0 };
	m_killerHeadPos = { 0,0,0 };
}


void KillCamReceiverSystem::init(Netcode::PlayerID player, Camera* cam) {
	initBase(player);

	if (m_cam == nullptr) {
		m_cam = SAIL_NEW CameraController(cam);
	}
}

void KillCamReceiverSystem::handleIncomingData(const std::string& data) {
	m_replayData.savePacket(data);
}


// Prepare transform components for the next frame
void KillCamReceiverSystem::prepareUpdate() {
	for (auto e : entities) {
		e->getComponent<TransformComponent>()->prepareFixedUpdate();
	}
}

// Increments the indexes in the ring buffer once per tick and clears the next write-index
void KillCamReceiverSystem::update(float dt) {
	m_replayData.prepareWrite();
	m_replayData.prepareRead();
	m_notHoldingTorches[m_replayData.writeIndex].clear();
}

void KillCamReceiverSystem::updatePerFrame(float dt, float alpha) {
	// Helper function
	auto interpolate = [](float prev, float current, float alpha) {
		return (alpha * current) + ((1.0f - alpha) * prev);
	};

	// only update the camera's position if a player killed us
	if (m_idOfKillingProjectile == Netcode::UNINITIALIZED || m_killerPlayer == nullptr) {
		return;
	}

	AnimationComponent* animation = m_killerPlayer->getComponent<AnimationComponent>();
	TransformComponent* transform = m_killerPlayer->getComponent<TransformComponent>();

	if (m_killerPlayer != nullptr) {
		m_killerHeadPos = transform->getRenderMatrix(alpha) * glm::vec4(animation->headPositionLocalCurrent, 1.f);
	}


	if (m_trackingProjectile) {
		// Follow the killing piece of water in slow motion
		if (m_killerProjectile != nullptr) {
			m_projectilePos = m_killerProjectile->getComponent<TransformComponent>()->getInterpolatedTranslation(alpha);
		}

		const glm::vec3 headToProjectile = m_projectilePos - m_killerHeadPos;
		const glm::vec3 camDir = glm::normalize(headToProjectile);
		const glm::vec3 cameraPos = m_projectilePos - (0.7f * camDir) + glm::vec3(0.f, 0.3f, 0.f);

		m_cam->setCameraPosition(cameraPos);
		m_cam->setCameralookAt(m_projectilePos);
	} else {
		// Show the killer's perspective
		m_cam->setCameraPosition(m_killerHeadPos);

		const glm::quat rot = glm::angleAxis(-interpolate(animation->prevPitch, animation->pitch, alpha), glm::vec3(1, 0, 0));
		const glm::quat rotated = glm::normalize(rot * transform->getInterpolatedRotation(alpha));

		const glm::vec3 forwards = rotated * glm::vec3(0.f, 0.f, 1.f);

		m_cam->setCameraDirection(forwards);
	}
}

// Should only be called when the killcam is active
void KillCamReceiverSystem::processReplayData(float dt) {
	m_myKillCamData.prepareRead();
	processData(dt, m_myKillCamData.getTickData(), false);

	// If we've reached the end of the killcam we should end it
	if (m_myKillCamData.readIndex == m_myKillCamData.writeIndex) {
		m_myKillCamData.clear();

		EventDispatcher::Instance().emit(StopKillCamEvent(m_isFinalKillCam));
	}
}


#ifdef DEVELOPMENT
// NOTE: does not track the size of m_myKillCamData but that will only be used when the killcam is being played
unsigned int KillCamReceiverSystem::getByteSize() const {
	unsigned int size = BaseComponentSystem::getByteSize() + sizeof(*this);
	for (int i = 0; i < REPLAY_BUFFER_SIZE; i++) {
		const auto& queue = m_replayData.netcodeData[i];
		const size_t queueSize = queue.size();
		size += queueSize * sizeof(std::string);								// string structure size
		if (queueSize) {
			size += queueSize * queue.front().capacity() * sizeof(unsigned char);	// approximate string character length
		}
		size += (m_notHoldingTorches[i].size() * sizeof(unsigned int));
	}
	return size;
}
#endif


// TODO: Move dead entities to under the map
void KillCamReceiverSystem::destroyEntity(const Netcode::ComponentID entityID) {
	if (auto e = findFromNetID(entityID); e) {

		// Only destroy projectiles atm
		if (e->hasComponent<ProjectileComponent>()) {
			e->queueDestruction();
		}
		
		if (entityID == m_idOfKillingProjectile) {
			m_killerProjectile = nullptr;
		}

		return;
	}
	SAIL_LOG_WARNING("destoryEntity called but no matching entity found");
}

void KillCamReceiverSystem::enableSprinklers() {
	//ECS::Instance()->getSystem<SprinklerSystem>()->enableSprinklers();
}

// SHOULD REMAIN EMPTY FOR THE KILLCAM
void KillCamReceiverSystem::endMatch(const GameDataForOthersInfo& info) {}

void KillCamReceiverSystem::extinguishCandle(const Netcode::ComponentID candleId, const Netcode::PlayerID shooterID) {
	if (auto e = findFromNetID(candleId); e) {
		e->getComponent<CandleComponent>()->wasJustExtinguished = true;
		e->getComponent<CandleComponent>()->wasHitByPlayerID = shooterID;

		return;
	}
	SAIL_LOG_WARNING("extinguishCandle called but no matching candle entity found");
}

void KillCamReceiverSystem::hitBySprinkler(const Netcode::ComponentID candleOwnerID) {
	//waterHitPlayer(candleOwnerID, Netcode::MESSAGE_SPRINKLER_ID);
}

void KillCamReceiverSystem::igniteCandle(const Netcode::ComponentID candleID) {
	if (auto candle = findFromNetID(candleID); candle) {
		auto candleComp = candle->getComponent<CandleComponent>();
		if (candleComp && !candleComp->isLit) {
			candleComp->health = MAX_HEALTH;
			candleComp->respawns++;
			candleComp->downTime = 0.f;
			candleComp->isLit = true;
			candleComp->userReignition = false;
			candleComp->invincibleTimer = 1.5f;
		}
		return;
	} 
	SAIL_LOG_WARNING("igniteCandle called but no matching entity found");
}

// SHOULD REMAIN EMPTY FOR THE KILLCAM
void KillCamReceiverSystem::matchEnded() {}

void KillCamReceiverSystem::playerDied(const Netcode::ComponentID networkIdOfKilled, const KillInfo& info) {
	//destroyEntity(networkIdOfKilled);
	
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
	if (auto e = findFromNetID(id); e) {
		auto animation = e->getComponent<AnimationComponent>();
		if (animation) {
			animation->setAnimation(info.index);
			animation->animationTime = info.time;
			animation->prevPitch = animation->pitch;
			animation->pitch = info.pitch;
		}
		return;
	}
	SAIL_LOG_WARNING("setAnimation called but no matching entity found");
}

void KillCamReceiverSystem::setCandleHealth(const Netcode::ComponentID candleId, const float health) {
	if (auto e = findFromNetID(candleId); e) {
		auto candle    = e->getComponent<CandleComponent>();
		auto particles = e->getComponent<ParticleEmitterComponent>();
		auto lc        = e->getComponent<LightComponent>();

		if (candle && particles && lc) {
			candle->health = health;
			// Scale fire particles with health
			particles->spawnRate = 0.01f * (MAX_HEALTH / candle->health);

			if (candle->wasJustExtinguished) {
				candle->health = 0.0f;
				candle->isLit = false;
				candle->wasJustExtinguished = false; // reset for the next tick
			}

			// COLOR/INTENSITY
			float tempHealthRatio = (std::fmaxf(candle->health, 0.f) / MAX_HEALTH);
			lc->getPointLight().setColor(tempHealthRatio * lc->defaultColor);
		}
		return;
	}
	SAIL_LOG_WARNING("setCandleHelath called but no matching candle entity found");
}

// The player who puts down their candle does this in CandleSystem and tests collisions
// The candle will be moved for everyone else in here
void KillCamReceiverSystem::setCandleState(const Netcode::ComponentID id, const bool isHeld) {
	Entity* player = nullptr;
	Entity* candle = nullptr;

	// Find the candle whose parent has the correct ID
	for (auto candleEntity : entities) {
		if (auto parentEntity = candleEntity->getParent(); parentEntity) {
			auto rrc = parentEntity->getComponent<ReplayReceiverComponent>();
			if (rrc && rrc->m_id == id) {
				player = parentEntity;
				candle = candleEntity;
				break;
			}
		}
	}

	// candle exists => player exists (only need to check candle)
	if (!candle) {
		Logger::Warning("Holding candle toggled but no matching candle entity found");
		return;
	}
	if (!player) {
		Logger::Warning("Holding candle toggled but no matching player entity found");
		return;
	}

	auto candleComp = candle->getComponent<CandleComponent>();
	auto candleTransComp = candle->getComponent<TransformComponent>();

	candleComp->isCarried = isHeld;
	candleComp->wasCarriedLastUpdate = isHeld;
	if (isHeld) {
		candleTransComp->setTranslation(glm::vec3(10.f, 2.0f, 0.f));
		candleTransComp->setParent(player->getComponent<TransformComponent>());

		player->getComponent<AnimationComponent>()->rightHandEntity = candle;
	} else {
		candleTransComp->removeParent();
		player->getComponent<AnimationComponent>()->rightHandEntity = nullptr;

		// Might be needed
		ECS::Instance()->getSystem<UpdateBoundingBoxSystem>()->update(0.0f);
	}
}

// Might need some optimization (like sorting) if we have a lot of networked entities
void KillCamReceiverSystem::setLocalPosition(const Netcode::ComponentID id, const glm::vec3& translation) {
	if (auto e = findFromNetID(id); e) {
		e->getComponent<TransformComponent>()->setTranslation(translation);
		return;
	}
	SAIL_LOG_WARNING("setLocalPosition called but no matching entity found");
}

void KillCamReceiverSystem::setLocalRotation(const Netcode::ComponentID id, const glm::vec3& rotation) {
	if (auto e = findFromNetID(id); e) {
		e->getComponent<TransformComponent>()->setRotations(rotation);
		return;
	}
	SAIL_LOG_WARNING("setLocalRotation called but no matching entity found");
}

void KillCamReceiverSystem::setLocalRotation(const Netcode::ComponentID id, const glm::quat& rotation) {
	if (auto e = findFromNetID(id); e) {
		e->getComponent<TransformComponent>()->setRotations(rotation);
		return;
	}
	SAIL_LOG_WARNING("setLocalRotation called but no matching entity found");
}

// SHOULD REMAIN EMPTY FOR THE KILLCAM
void KillCamReceiverSystem::setPlayerStats(const PlayerStatsInfo& info) {}

// SHOULD PROABABLY REMAIN EMPTY FOR THE KILLCAM
void KillCamReceiverSystem::updateSanity(const Netcode::ComponentID id, const float sanity) {}

void KillCamReceiverSystem::updateProjectile(const Netcode::ComponentID id, const glm::vec3& pos, const glm::vec3& vel) {
	if (auto e = findFromNetID(id); e) {
		e->getComponent<TransformComponent>()->setTranslation(pos);
		e->getComponent<MovementComponent>()->velocity = vel;
		return;
	}
	SAIL_LOG_WARNING("updateProjectile called but no matching entity found");
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
	args.lifetime *= SLOW_MO_MULTIPLIER;

	EntityFactory::CreateReplayProjectile(e, args);

	// Start the slow motion when the killing projectile spawns
	if (info.projectileID == m_idOfKillingProjectile) {
		m_slowMotionState = SlowMotionSetting::ENABLE;
		m_trackingProjectile = true;
		m_killerProjectile = e.get();
	}
}

void KillCamReceiverSystem::submitWaterPoint(const glm::vec3& point) {
}


// kill player when water hits it
void KillCamReceiverSystem::waterHitPlayer(const Netcode::ComponentID id, const Netcode::ComponentID killerID) {
	//EventDispatcher::Instance().emit(WaterHitPlayerEvent(id, senderId));
	//if (killerID == m_idOfKillingProjectile && Netcode::getComponentOwner(id) == m_playerID) {
	//	destroyEntity(id);
	//}
}



// AUDIO

void KillCamReceiverSystem::playerJumped(const Netcode::ComponentID id) {
	//EventDispatcher::Instance().emit(PlayerJumpedEvent(id));
}

void KillCamReceiverSystem::playerLanded(const Netcode::ComponentID id) {
	//EventDispatcher::Instance().emit(PlayerLandedEvent(id));
}


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
		if (e->getComponent<ReplayReceiverComponent>()->m_id == id) {
			return e;
		}
	}
	return nullptr;
}


bool KillCamReceiverSystem::onEvent(const Event& event) {
	auto onStartKillCam = [&](const StartKillCamEvent& e) {
		if (m_isFinalKillCam) { return; } // Ignore this event if we're already in the final killcam

		stopMyKillCam();

		// Note: PlayerSystem will have checked that this is the ID of a projectile for us
		m_idOfKillingProjectile = e.killingProjectile;
		m_isFinalKillCam = e.finalKillCam;

		startKillCam();
	};

	auto onToggleSlowMotion = [&](const ToggleSlowMotionReplayEvent& e) {
		m_slowMotionState = e.setting;
	};

	auto onTorchNotHeld = [&](const TorchNotHeldEvent& e) {
		if (!m_hasStarted) {
			m_notHoldingTorches[m_replayData.writeIndex].push_back(e.netCompID);
		}
	};

	switch (event.type) {
	case Event::Type::TOGGLE_SLOW_MOTION: onToggleSlowMotion((const ToggleSlowMotionReplayEvent&)event); break;
	case Event::Type::TORCH_NOT_HELD:     onTorchNotHeld((const TorchNotHeldEvent&)event); break;
	case Event::Type::START_KILLCAM:      onStartKillCam((const StartKillCamEvent&)event); break;
	default: break;
	}

	return true;
}