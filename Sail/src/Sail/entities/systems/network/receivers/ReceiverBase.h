
#pragma once

#include "../../BaseComponentSystem.h"
#include "Sail/netcode/NetworkedStructs.h"

#include "glm/gtc/quaternion.hpp"

class GameState;
class NetworkSenderSystem;

class ReceiverBase : public BaseComponentSystem {
public: // Functions
	ReceiverBase();
	virtual ~ReceiverBase();

	virtual void stop() override;

	void setPlayer(Entity* player);
	void setGameState(GameState* gameState);
	const std::vector<Entity*>& getEntities() const;

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
		float pitch;
	};

	struct EndScreenInfo {
		int bulletsFired;
		int jumpsMade;
		float distanceWalked;
	};

	struct GameDataForOthersInfo {
		int bulletsFired;
		Netcode::PlayerID bulletsFiredID;
		float distanceWalked;
		Netcode::PlayerID distanceWalkedID;
		int jumpsMade;
		Netcode::PlayerID jumpsMadeID;
	};
#pragma endregion

protected: // Functions
	void initBase(Netcode::PlayerID playerID);


	virtual void createPlayer    (const PlayerComponentInfo& info, const glm::vec3& pos)                  = 0;
	virtual void destroyEntity   (const Netcode::ComponentID entityID)                                    = 0;
	virtual void enableSprinklers()                                                                       = 0;
	virtual void endMatch        (const GameDataForOthersInfo& info)                                      = 0;
	virtual void extinguishCandle(const Netcode::ComponentID candleID, const Netcode::PlayerID shooterID) = 0;
	virtual void hitBySprinkler  (const Netcode::ComponentID candleOwnerID)                               = 0;
	virtual void igniteCandle    (const Netcode::ComponentID candleID)                                    = 0;
	virtual void matchEnded      ()                                                                       = 0;
	virtual void playerDied      (const Netcode::ComponentID id, const Netcode::ComponentID killerID)     = 0;
	virtual void setAnimation    (const Netcode::ComponentID id, const AnimationInfo& info)               = 0;
	virtual void setCandleHealth (const Netcode::ComponentID candleID, const float health)                = 0;
	virtual void setCandleState  (const Netcode::ComponentID id, const bool isHeld)                       = 0;
	virtual void setLocalPosition(const Netcode::ComponentID id, const glm::vec3& pos)                    = 0;
	virtual void setLocalRotation(const Netcode::ComponentID id, const glm::vec3& rot)                    = 0;
	virtual void setLocalRotation(const Netcode::ComponentID id, const glm::quat& rot)                    = 0;
	virtual void setPlayerStats  (Netcode::PlayerID player, int nrOfKills, int placement)                 = 0;
	virtual void updateSanity    (const Netcode::ComponentID id, const float sanity)                      = 0;
	virtual void spawnProjectile (const ProjectileInfo& info)                                             = 0;
	virtual void waterHitPlayer  (const Netcode::ComponentID id, const Netcode::ComponentID projectileID) = 0;

	// AUDIO
	virtual void playerJumped (const Netcode::ComponentID id)                = 0;
	virtual void playerLanded (const Netcode::ComponentID id)                = 0;
	virtual void shootStart (const Netcode::ComponentID id, float frequency) = 0;
	virtual void shootLoop  (const Netcode::ComponentID id, float frequency) = 0;
	virtual void shootEnd   (const Netcode::ComponentID id, float frequency) = 0;
	virtual void runningMetalStart     (const Netcode::ComponentID id)       = 0;
	virtual void runningWaterMetalStart(const Netcode::ComponentID id)       = 0;
	virtual void runningTileStart      (const Netcode::ComponentID id)       = 0;
	virtual void runningWaterTileStart (const Netcode::ComponentID id)       = 0;
	virtual void runningStopSound      (const Netcode::ComponentID id)       = 0;
	virtual void throwingStartSound    (const Netcode::ComponentID id)       = 0;
	virtual void throwingEndSound	   (const Netcode::ComponentID id)       = 0;

	// FUNCTIONS THAT DIFFER BETWEEN HOST AND CLIENT
	
	// HOST ONLY FUNCTIONS
	virtual void endMatchAfterTimer(const float dt) = 0; // Made for the host to quit the game after a set time
	virtual void mergeHostsStats()                  = 0; // Host adds its data to global statistics before waiting for clients
	virtual void prepareEndScreen(const Netcode::PlayerID sender, const EndScreenInfo& info) = 0;


	// NOT FROM SERIALIZED MESSAGES
	virtual void playerDisconnect(const Netcode::PlayerID playerID) = 0;
	
	// Helper function
	virtual Entity* findFromNetID(const Netcode::ComponentID id) const = 0;

protected:
	// The player entity is used to prevent creation of receiver components for entities controlled by the player
	Netcode::PlayerID m_playerID;
	Entity* m_playerEntity;

	GameState* m_gameStatePtr;
};
