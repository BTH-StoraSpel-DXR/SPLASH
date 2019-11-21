#pragma once

#include "ReceiverBase.h"
#include "../../BaseComponentSystem.h"
#include "Sail/netcode/NetworkedStructs.h"
#include "Sail/events/EventReceiver.h"
#include "Sail/events/types/ToggleSlowMotionReplayEvent.h"

#include <array>
#include "Sail/TimeSettings.h"

class GameState;
class NetworkSenderSystem;
class GameDataTracker;

class KillCamReceiverSystem : public ReceiverBase, public EventReceiver {
public:
	// Packets from the past five seconds are saved so that they can be replayed in the killcam.
	static constexpr size_t REPLAY_BUFFER_SIZE = TICKRATE * 5;
	static constexpr size_t SLOW_MO_MULTIPLIER = 4;

public:
	KillCamReceiverSystem();
	virtual ~KillCamReceiverSystem();

	void init(Netcode::PlayerID player);
	void handleIncomingData(const std::string& data) override;
	void update (float dt) override;
	void stop() override;

	void prepareUpdate();
	void processReplayData(float dt);


	float getKillCamAlpha(const float alpha) const {
		if (m_slowMotionState == SlowMotionSetting::ENABLE) {
			return (static_cast<float>(m_killCamTickCounter) + alpha) / static_cast<float>(SLOW_MO_MULTIPLIER);
		} else {
			return alpha;
		}
	}

	// If slow motion is enabled only update once every SLOW_MO_MULTIPLIER ticks
	bool skipUpdate() {
		m_killCamTickCounter = (m_killCamTickCounter + 1) % SLOW_MO_MULTIPLIER;
		return (m_slowMotionState == SlowMotionSetting::ENABLE && m_killCamTickCounter != 0);
	}

#ifdef DEVELOPMENT
	unsigned int getByteSize() const override;
	void imguiPrint(Entity** selectedEntity = nullptr) {

		ImGui::Text(std::string("ID: " + std::to_string((int)m_playerID)).c_str());
	}
#endif
private:
	void createPlayer    (const PlayerComponentInfo& info, const glm::vec3& pos)                  override;
	void destroyEntity   (const Netcode::ComponentID entityID)                                    override;
	void enableSprinklers()                                                                       override;
	void endMatch        (const GameDataForOthersInfo& info)                                      override;
	void extinguishCandle(const Netcode::ComponentID candleID, const Netcode::PlayerID shooterID) override;
	void hitBySprinkler  (const Netcode::ComponentID candleOwnerID)                               override;
	void igniteCandle    (const Netcode::ComponentID candleID)                                    override;
	void matchEnded      ()                                                                       override;
	void playerDied      (const Netcode::ComponentID id, const Netcode::ComponentID killerID)     override;
	void setAnimation    (const Netcode::ComponentID id, const AnimationInfo& info)               override;
	void setCandleHealth (const Netcode::ComponentID candleID, const float health)                override;
	void setCandleState  (const Netcode::ComponentID id, const bool isHeld)                       override;
	void setLocalPosition(const Netcode::ComponentID id, const glm::vec3& pos)                    override;
	void setLocalRotation(const Netcode::ComponentID id, const glm::vec3& rot)                    override;
	void setLocalRotation(const Netcode::ComponentID id, const glm::quat& rot)                    override;
	void setPlayerStats  (Netcode::PlayerID player, int nrOfKills, int placement)                 override;
	void updateSanity    (const Netcode::ComponentID id, const float sanity)                      override;
	void spawnProjectile (const ProjectileInfo& info)                                             override;
	void waterHitPlayer  (const Netcode::ComponentID id, const Netcode::ComponentID killerID)     override;

	// AUDIO
	void playerJumped (const Netcode::ComponentID id)                  override;
	void playerLanded (const Netcode::ComponentID id)                  override;
	void shootStart   (const Netcode::ComponentID id, float frequency) override;
	void shootLoop    (const Netcode::ComponentID id, float frequency) override;
	void shootEnd     (const Netcode::ComponentID id, float frequency) override;
	void runningMetalStart     (const Netcode::ComponentID id)         override;
	void runningWaterMetalStart(const Netcode::ComponentID id)         override;
	void runningTileStart      (const Netcode::ComponentID id)         override;
	void runningWaterTileStart (const Netcode::ComponentID id)         override;
	void runningStopSound      (const Netcode::ComponentID id)         override;
	void throwingStartSound	   (const Netcode::ComponentID id)         override;
	void throwingEndSound	   (const Netcode::ComponentID id)         override;

	// HOST ONLY
	void endMatchAfterTimer(const float dt) override; // Made for the host to quit the game after a set time
	void mergeHostsStats()                  override; // Host adds its data to global statistics before waiting for clients
	void prepareEndScreen(const Netcode::PlayerID sender, const EndScreenInfo& info) override;

	// NOT FROM SERIALIZED MESSAGES
	void playerDisconnect(const Netcode::PlayerID playerID) override;
	
	// Helper function
	Entity* findFromNetID(const Netcode::ComponentID id) const override;


	bool onEvent(const Event& event) override;

private:
	// All the messages that have been sent/received over the network in the past few seconds
	// Will be used like a ring buffer
	std::array<std::queue<std::string>, REPLAY_BUFFER_SIZE> m_replayData;
	std::mutex m_replayDataLock;

	size_t m_currentWriteInd = 0;
	size_t m_currentReadInd  = 1;
	bool   m_hasStarted      = false;

	SlowMotionSetting m_slowMotionState = SlowMotionSetting::ENABLE;
	size_t m_killCamTickCounter = 0; // Counts ticks in the range [ 0, SLOW_MO_MULTIPLIER )


	Netcode::ComponentID m_idOfKillingProjectile = 0;
};
