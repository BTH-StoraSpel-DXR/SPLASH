#pragma once

#include "ReceiverBase.h"
#include "../../BaseComponentSystem.h"
#include "Sail/netcode/NetworkedStructs.h"
#include "Sail/events/EventReceiver.h"

class GameState;
class NetworkSenderSystem;
class GameDataTracker;

class NetworkReceiverSystem : public ReceiverBase {
public:
	NetworkReceiverSystem();
	virtual ~NetworkReceiverSystem();

	void pushDataToBuffer(const std::string& data);

	void update(float dt) override;

#ifdef DEVELOPMENT
	void imguiPrint(Entity** selectedEntity = nullptr) {

		ImGui::Text(std::string("ID: " + std::to_string((int)m_playerID)).c_str());
	}
#endif

protected:
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
	void shootStart   (const Netcode::ComponentID id, float frequency)			 override;
	void shootLoop	  (const Netcode::ComponentID id, float frequency)			 override;
	void shootEnd	  (const Netcode::ComponentID id, float frequency)			 override;
	void runningMetalStart     (const Netcode::ComponentID id)                   override;
	void runningWaterMetalStart(const Netcode::ComponentID id)                   override;
	void runningTileStart      (const Netcode::ComponentID id)                   override;
	void runningWaterTileStart (const Netcode::ComponentID id)                   override;
	void runningStopSound      (const Netcode::ComponentID id)                   override;
	void throwingStartSound	   (const Netcode::ComponentID id)					 override;
	void throwingEndSound	   (const Netcode::ComponentID id)					 override;

	// NOT FROM SERIALIZED MESSAGES
	void playerDisconnect(const Netcode::PlayerID playerID) override;
	
	// Helper function
	Entity* findFromNetID(const Netcode::ComponentID id) const override;

	bool onEvent(const Event& event) override;

protected:
	// FIFO container of serialized data-strings to decode
	std::queue<std::string> m_incomingDataBuffer;
	std::mutex m_bufferLock;
};
