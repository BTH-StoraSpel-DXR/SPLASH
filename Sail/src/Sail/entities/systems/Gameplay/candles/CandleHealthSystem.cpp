#include "pch.h"
#include "CandleHealthSystem.h"
#include "Sail/entities/components/Components.h"
#include "Network/NWrapperSingleton.h"
#include "Sail/utils/GameDataTracker.h"

#include "Sail/events/EventDispatcher.h"
#include "Sail/events/types/WaterHitPlayerEvent.h"

CandleHealthSystem::CandleHealthSystem() {
	registerComponent<CandleComponent>(true, true, true);
	registerComponent<NetworkSenderComponent>(false, true, false);
	registerComponent<LightComponent>(true, true, true);
	registerComponent<AudioComponent>(false, true, true);

	EventDispatcher::Instance().subscribe(Event::Type::WATER_HIT_PLAYER, this);
}

CandleHealthSystem::~CandleHealthSystem() {
	EventDispatcher::Instance().unsubscribe(Event::Type::WATER_HIT_PLAYER, this);
}

void CandleHealthSystem::update(float dt) {
	const bool isHost = NWrapperSingleton::getInstance().isHost();

	// The number of living candles, representing living players
	size_t livingCandles = entities.size();

	for (auto e : entities) {
		auto candle = e->getComponent<CandleComponent>();
		candle->wasHitByMeThisTick = false;

		// Scale fire particles with health
		auto particles = e->getComponent<ParticleEmitterComponent>();
		particles->spawnRate = 0.01f * (MAX_HEALTH / candle->health);

#pragma region HOST_ONLY_STUFF
		if (isHost && candle->isLit) {
			if (candle->health > 0.0f) {
				candle->invincibleTimer -= dt;

				// If someone hit the candle this tick tell all players to update the candle's health
				if (candle->wasHitThisTick) {
					// Candle has lost all its health so extinguish it
					NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
						Netcode::MessageType::SET_CANDLE_HEALTH,
						SAIL_NEW Netcode::MessageSetCandleHealth{
							e->getComponent<NetworkReceiverComponent>()->m_id,
							candle->health
						},
						false // Host already knows the candle's health so don't send to ourselves
					);
					candle->wasHitThisTick = false;
				}
			} else { // If candle used to be lit but has lost all its health
				candle->wasJustExtinguished = true;

				// Candle has lost all its health so extinguish it
				NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
					Netcode::MessageType::EXTINGUISH_CANDLE,
					SAIL_NEW Netcode::MessageExtinguishCandle{
						e->getComponent<NetworkReceiverComponent>()->m_id,
						candle->wasHitByPlayerID
					},
					false // Host extinguishes candle later in this function so don't send to ourself
				);

				// If the player has no more respawns kill them
				if (candle->respawns >= m_maxNumRespawns) {
					livingCandles--;

					NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
						Netcode::MessageType::PLAYER_DIED,
						SAIL_NEW Netcode::MessagePlayerDied{
							e->getParent()->getComponent<NetworkReceiverComponent>()->m_id,
							candle->wasHitByPlayerID
						},
						true
					);

					// Save the placement for the player who lost
					GameDataTracker::getInstance().logPlacement(
						Netcode::getComponentOwner(e->getParent()->getComponent<NetworkReceiverComponent>()->m_id)
					);

					// Only one living candle left and number of players in the game is greater than one
					if (livingCandles < 2 && entities.size() > 1) {

						for (auto e2 : entities) {
							NetworkReceiverComponent* cc = e2->getParent()->getComponent<NetworkReceiverComponent>();
							if (cc->m_id != e->getParent()->getComponent<NetworkReceiverComponent>()->m_id) {
								// Save the placement for the player who lost
								GameDataTracker::getInstance().logPlacement(
									Netcode::getComponentOwner(cc->m_id)
								);
							}
						}

						NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
							Netcode::MessageType::MATCH_ENDED,
							nullptr
						);
					}
				}
			}
		}
#pragma endregion


		// Everyone does this
		if (candle->wasJustExtinguished) {
			candle->health = 0.0f;
			candle->isLit = false;
			candle->wasJustExtinguished = false; // reset for the next tick

			if (candle->wasHitByPlayerID < Netcode::NONE_PLAYER_ID_START) {
				GameDataTracker::getInstance().logEnemyKilled(candle->wasHitByPlayerID);
			}

			else if (candle->wasHitByPlayerID == Netcode::MESSAGE_INSANITY_ID) {
				e->getParent()->getComponent<AudioComponent>()->m_sounds[Audio::INSANITY_SCREAM].isPlaying = true;
			}
		
			// Play the reignition sound if the player has any candles left
			if (candle->respawns < m_maxNumRespawns) {
				auto playerEntity = e->getParent();
				playerEntity->getComponent<AudioComponent>()->m_sounds[Audio::RE_IGNITE_CANDLE].isPlaying = true;
			}
		}

		// COLOR/INTENSITY
		float tempHealthRatio = (std::fmaxf(candle->health, 0.f) / MAX_HEALTH);

		LightComponent* lc = e->getComponent<LightComponent>();

		lc->getPointLight().setColor(tempHealthRatio * lc->defaultColor);
	}
}

bool CandleHealthSystem::onEvent(const Event& event) {
	auto findCandleFromParentID = [=](const Netcode::ComponentID netCompID) {
		Entity* candle = nullptr;
		for (auto entity : entities) {
			if (auto parent = entity->getParent(); parent) {
				if (parent->getComponent<NetworkReceiverComponent>()->m_id == netCompID) {
					candle = entity;
					break;
				}
			}
		}
		return candle;
	};

	auto onWaterHitPlayer = [=](const WaterHitPlayerEvent& e) {
		Entity* candle = findCandleFromParentID(e.netCompID);

		if (!candle) {
			Logger::Warning("CandleHealthSystem::onWaterHitPlayer: no matching entity found");
			return;
		}

		// Damage the candle
		// TODO: Replace 10.0f with game settings damage
		if (e.senderID == Netcode::MESSAGE_SPRINKLER_ID) {
			candle->getComponent<CandleComponent>()->hitWithWater(1.0f, CandleComponent::DamageSource::PLAYER, e.senderID);
		}
		else {
			candle->getComponent<CandleComponent>()->hitWithWater(10.0f, CandleComponent::DamageSource::PLAYER, e.senderID);

		}
	};

	switch (event.type) {
	case Event::Type::WATER_HIT_PLAYER: onWaterHitPlayer((const WaterHitPlayerEvent&)event); break;
	default: break;
	}

	return true;
}
