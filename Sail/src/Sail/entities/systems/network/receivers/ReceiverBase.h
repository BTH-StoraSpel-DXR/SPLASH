#pragma once

#include "../../BaseComponentSystem.h"
#include "Sail/netcode/NetworkedStructs.h"
#include "Sail/events/EventReceiver.h"

#include "glm/gtc/quaternion.hpp"

class GameState;
class NetworkSenderSystem;
class GameDataTracker;

class ReceiverBase : public BaseComponentSystem, public EventReceiver {
public: // Functions
	ReceiverBase();
	virtual ~ReceiverBase();

	// TODO: See which of these functions need to be in base and which ones are only used
	//       by the network receiver systems and not by the killcam receiver system
	void init(Netcode::PlayerID playerID, NetworkSenderSystem* netSendSysPtr);
	void setPlayer(Entity* player);
	void setGameState(GameState* gameState);
	const std::vector<Entity*>& getEntities() const;
	
	// TODO: MOVE TO NRS
	//void pushDataToBuffer(const std::string& data);

	void processData(float dt, std::queue<std::string>& data, const bool ignoreFromSelf = true);

	virtual void update(float dt) = 0;
	virtual void handleIncomingData(const std::string& data) = 0;

#pragma region INPUT_PARAMS
protected:
	struct PlayerComponentInfo {
		Netcode::ComponentID playerCompID;
		Netcode::ComponentID candleID;
		Netcode::ComponentID gunID;
	};

	struct ProjectileInfo {
		Netcode::ComponentID projectileID;
		Netcode::ComponentID ownerID;
		glm::vec3 position;
		glm::vec3 velocity;
	};

	struct AnimationInfo {
		unsigned int index;
		float time;
	};

	struct EndScreenInfo {
		int bulletsFired;
		int jumpsMade;
		float distanceWalked;
	};
#pragma endregion

protected: // Functions
	virtual void createPlayer    (const PlayerComponentInfo& info, const glm::vec3& pos)                  = 0;
	virtual void destroyEntity   (const Netcode::ComponentID entityID)                                    = 0;
	virtual void enableSprinklers()                                                                       = 0;
	virtual void extinguishCandle(const Netcode::ComponentID candleID, const Netcode::PlayerID shooterID) = 0;
	virtual void hitBySprinkler  (const Netcode::ComponentID candleOwnerID)                               = 0;
	virtual void igniteCandle    (const Netcode::ComponentID candleID)                                    = 0;
	virtual void playerDied      (const Netcode::ComponentID id, const Netcode::PlayerID shooterID)       = 0;
	virtual void playerJumped    (const Netcode::ComponentID id)                                          = 0;
	virtual void playerLanded    (const Netcode::ComponentID id)                                          = 0;
	virtual void setAnimation    (const Netcode::ComponentID id, const AnimationInfo& info)               = 0;
	virtual void setCandleHealth (const Netcode::ComponentID candleID, const float health)                = 0;
	virtual void setCandleState  (const Netcode::ComponentID id, const bool isHeld)                       = 0;
	virtual void setLocalPosition(const Netcode::ComponentID id, const glm::vec3& pos)                    = 0;
	virtual void setLocalRotation(const Netcode::ComponentID id, const glm::vec3& rot)                    = 0;
	virtual void setLocalRotation(const Netcode::ComponentID id, const glm::quat& rot)                    = 0;
	virtual void spawnProjectile (const ProjectileInfo& info)                                             = 0;
	virtual void waterHitPlayer  (const Netcode::ComponentID id, const Netcode::PlayerID SenderId)        = 0;

	// AUDIO
	virtual void shootStart (const Netcode::ComponentID id, float frequency)		   = 0;
	virtual void shootLoop  (const Netcode::ComponentID id, float frequency)		   = 0;
	virtual void shootEnd   (const Netcode::ComponentID id, float frequency)		   = 0;
	virtual void runningMetalStart     (const Netcode::ComponentID id)                 = 0;
	virtual void runningWaterMetalStart(const Netcode::ComponentID id)                 = 0;
	virtual void runningTileStart      (const Netcode::ComponentID id)                 = 0;
	virtual void runningWaterTileStart (const Netcode::ComponentID id)                 = 0;
	virtual void runningStopSound      (const Netcode::ComponentID id)                 = 0;
	virtual void throwingStartSound    (const Netcode::ComponentID id)				   = 0;
	virtual void throwingEndSound	   (const Netcode::ComponentID id)				   = 0;

	// FUNCTIONS THAT DIFFER BETWEEN HOST AND CLIENT
	virtual void endMatch() = 0;
	
	// HOST ONLY FUNCTIONS
	virtual void endMatchAfterTimer(const float dt) = 0; // Made for the host to quit the game after a set time
	virtual void mergeHostsStats()                  = 0; // Host adds its data to global statistics before waiting for clients
	virtual void prepareEndScreen(const Netcode::PlayerID sender, const EndScreenInfo& info) = 0;

	//bool onEvent(const Event& event) override;


	// NOT FROM SERIALIZED MESSAGES
	virtual void playerDisconnect(const Netcode::PlayerID playerID) = 0;
	
	// Helper function
	virtual Entity* findFromNetID(const Netcode::ComponentID id) const = 0;

protected:
	NetworkSenderSystem* m_netSendSysPtr;
	GameDataTracker* m_gameDataTracker; // needed?

	// The player entity is used to prevent creation of receiver components for entities controlled by the player
	Netcode::PlayerID m_playerID; // TODO? move to NRS
	Entity* m_playerEntity;

	GameState* m_gameStatePtr;
};
