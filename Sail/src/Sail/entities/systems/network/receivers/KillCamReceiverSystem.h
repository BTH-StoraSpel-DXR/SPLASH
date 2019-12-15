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
class Camera;
class CameraController;

class KillCamReceiverSystem : public ReceiverBase, public EventReceiver {
public:
	// Packets from the past five seconds are saved so that they can be replayed in the killcam.
	static constexpr size_t KILLCAM_DURATION   = 3;
	static constexpr size_t REPLAY_BUFFER_SIZE = TICKRATE * KILLCAM_DURATION;
	static constexpr size_t SLOW_MO_MULTIPLIER = 20;

public:
	KillCamReceiverSystem();
	virtual ~KillCamReceiverSystem();

	
	float getKillCamAlpha(const float alpha) const;
	float getKillCamDelta(const float delta) const;
	bool skipUpdate(); // If slow motion is enabled only update once every SLOW_MO_MULTIPLIER ticks


	void init(Netcode::PlayerID player, Camera* cam);
	void handleIncomingData(const std::string& data) override;
	void update (float dt) override;
	void updatePerFrame(float dt, float alpha);
	void stop() override;


	void prepareUpdate();
	void processReplayData(float dt);



#ifdef DEVELOPMENT
	unsigned int getByteSize() const override;
	void imguiPrint(Entity** selectedEntity = nullptr) {

		ImGui::Text(std::string("ID: " + std::to_string((int)m_playerID)).c_str());
	}
#endif
private:
	void initEntities();
	bool startKillCam();
	void stopMyKillCam();

	bool onEvent(const Event& event) override;


	void destroyEntity   (const Netcode::ComponentID entityID)                                       override;
	void enableSprinklers()                                                                          override;
	void endMatch        (const GameDataForOthersInfo& info)                                         override;
	void extinguishCandle(const Netcode::ComponentID candleID, const Netcode::PlayerID shooterID)    override;
	void hitBySprinkler  (const Netcode::ComponentID candleOwnerID)                                  override;
	void igniteCandle    (const Netcode::ComponentID candleID)                                       override;
	void matchEnded      ()                                                                          override;
	void playerDied      (const Netcode::ComponentID id, const KillInfo& info)                       override;
	void setAnimation    (const Netcode::ComponentID id, const AnimationInfo& info)                  override;
	void setCandleHealth (const Netcode::ComponentID candleID, const float health)                   override;
	void setCandleState  (const Netcode::ComponentID id, const bool isHeld)                          override;
	void setLocalPosition(const Netcode::ComponentID id, const glm::vec3& pos)                       override;
	void setLocalRotation(const Netcode::ComponentID id, const glm::vec3& rot)                       override;
	void setLocalRotation(const Netcode::ComponentID id, const glm::quat& rot)                       override;
	void setPlayerStats  (const PlayerStatsInfo& info)                                               override;
	void updateSanity    (const Netcode::ComponentID id, const float sanity)                         override;
	void updateProjectile(const Netcode::ComponentID id, const glm::vec3& pos, const glm::vec3& vel) override;
	void spawnProjectile (const ProjectileInfo& info)                                                override;
	void submitWaterPoint(const glm::vec3& point)                                                    override;
	void waterHitPlayer  (const Netcode::ComponentID id, const Netcode::ComponentID killerID)        override;
	void setCenter(const Netcode::ComponentID compID, const glm::vec3 offset)                        override;

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



private:
	struct NetcodeDataRingBuffer {
		std::array<std::queue<std::string>, REPLAY_BUFFER_SIZE> netcodeData;
		size_t writeIndex = 0;
		size_t readIndex = 1;

		void clear() {
			for (std::queue<std::string>& data : netcodeData) { data = std::queue<std::string>(); }
			writeIndex = 0;
			readIndex = 1;
		}

		void prepareWrite() {
			writeIndex = ++writeIndex % REPLAY_BUFFER_SIZE;
			netcodeData[writeIndex] = std::queue<std::string>();
		}

		void prepareRead() { 
			readIndex  = ++readIndex % REPLAY_BUFFER_SIZE;
		}

		void savePacket(const std::string& data) {
			netcodeData[writeIndex].push(data);
		}

		std::queue<std::string>& getTickData() {
			return netcodeData[readIndex];
		}
	};


	// All the messages that have been sent/received over the network in the past few seconds
	// Will be used like a ring buffer
	NetcodeDataRingBuffer m_replayData;
	std::array<std::vector<Netcode::ComponentID>, REPLAY_BUFFER_SIZE> m_notHoldingTorches;

	// Copies the current state of m_replayData so that we can keep recording to m_replayData and play our killcam at
	// the same time
	NetcodeDataRingBuffer m_myKillCamData;	

	bool m_hasInitialized = false;
	bool m_isPlaying      = false;
	bool m_isFinalKillCam = false;
	bool m_trackingProjectile  = false;

	SlowMotionSetting m_slowMotionState = SlowMotionSetting::DISABLE;
	size_t m_killCamTickCounter = 0; // Counts ticks in the range [ 0, SLOW_MO_MULTIPLIER )

	Netcode::ComponentID m_killingProjectileID = Netcode::UNINITIALIZED;

	Entity* m_killerPlayer     = nullptr;
	Entity* m_killerProjectile = nullptr;

	CameraController* m_cam = nullptr;

	glm::vec3 m_projectilePos = { 0,0,0 };
	glm::vec3 m_killerHeadPos = { 0,0,0 };

};
