#pragma once

#include "ReceiverBase.h"
#include "../../BaseComponentSystem.h"
#include "Sail/netcode/NetworkedStructs.h"
#include "Sail/events/EventReceiver.h"

#include <array>
#include "Sail/TimeSettings.h"

// Packets from the past five seconds are saved so that they can be replayed in the killcam.
constexpr size_t REPLAY_BUFFER_SIZE = TICKRATE * 5;

class GameState;
class NetworkSenderSystem;
class GameDataTracker;

class KillCamReceiverSystem : public ReceiverBase {
public:
	KillCamReceiverSystem();
	virtual ~KillCamReceiverSystem();

	void handleIncomingData(const std::string& data) override;
	void update            (float dt)          override;

	void processReplayData(float dt);


#ifdef DEVELOPMENT
	void imguiPrint(Entity** selectedEntity = nullptr) {

		ImGui::Text(std::string("ID: " + std::to_string((int)m_playerID)).c_str());
	}
#endif


private:
	void createPlayer    (const PlayerComponentInfo& info, const glm::vec3& pos)                  override;
	void destroyEntity   (const Netcode::ComponentID entityID)                                    override;
	void enableSprinklers()                                                                       override;
	void extinguishCandle(const Netcode::ComponentID candleID, const Netcode::PlayerID shooterID) override;
	void hitBySprinkler  (const Netcode::ComponentID candleOwnerID)                               override;
	void igniteCandle    (const Netcode::ComponentID candleID)                                    override;
	void playerDied      (const Netcode::ComponentID id, const Netcode::PlayerID shooterID)       override;
	void setAnimation    (const Netcode::ComponentID id, const AnimationInfo& info)               override;
	void setCandleHealth (const Netcode::ComponentID candleID, const float health)                override;
	void setCandleState  (const Netcode::ComponentID id, const bool isHeld)                       override;
	void setLocalPosition(const Netcode::ComponentID id, const glm::vec3& pos)                    override;
	void setLocalRotation(const Netcode::ComponentID id, const glm::vec3& rot)                    override;
	void setLocalRotation(const Netcode::ComponentID id, const glm::quat& rot)                    override;
	void spawnProjectile (const ProjectileInfo& info)                                             override;
	void waterHitPlayer  (const Netcode::ComponentID id, const Netcode::PlayerID SenderId)        override;

	// AUDIO
	void playerJumped (const Netcode::ComponentID id)                            override;
	void playerLanded (const Netcode::ComponentID id)                            override;
	void shootStart   (const Netcode::ComponentID id, const ShotFiredInfo& info) override;
	void shootLoop    (const Netcode::ComponentID id, const ShotFiredInfo& info) override;
	void shootEnd     (const Netcode::ComponentID id, const ShotFiredInfo& info) override;
	void runningMetalStart     (const Netcode::ComponentID id)                   override;
	void runningWaterMetalStart(const Netcode::ComponentID id)                   override;
	void runningTileStart      (const Netcode::ComponentID id)                   override;
	void runningWaterTileStart (const Netcode::ComponentID id)                   override;
	void runningStopSound      (const Netcode::ComponentID id)                   override;

	// HOST ONLY
	void endMatch()                         override; // Start end timer for host
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
	size_t m_currentWriteInd = 0;
	size_t m_currentReadInd = 1;

	std::mutex m_replayDataLock;
};
