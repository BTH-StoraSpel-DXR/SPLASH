#pragma once

#include "../../BaseComponentSystem.h"
#include "Sail/netcode/NetworkedStructs.h"

class GameState;
class NetworkSenderSystem;

class NetworkReceiverSystem : public BaseComponentSystem {
public:
	NetworkReceiverSystem();
	~NetworkReceiverSystem();

	// Functions which differ from host to client
	virtual void pushDataToBuffer(std::string data) = 0;

	void init(Netcode::PlayerID playerID, GameState* gameStatePtr, NetworkSenderSystem* netSendSysPtr);
	void initPlayer(Entity* pPlayerEntity);

	const std::vector<Entity*>& getEntities() const;

	void update();
protected:
	GameState* m_gameStatePtr;
	NetworkSenderSystem* m_netSendSysPtr;

	// FIFO container of serialized data-strings to decode
	std::queue<std::string> m_incomingDataBuffer;
	std::mutex m_bufferLock;

	// The player's ID is used to prevent creation of receiver components for entities controlled by the player
	Netcode::PlayerID m_playerID;

	Entity* m_playerEntity = nullptr;
private:
	void createEntity(Netcode::ComponentID id, Netcode::EntityType entityType, const glm::vec3& translation);
	void setEntityTranslation(Netcode::ComponentID id, const glm::vec3& translation);
	void setEntityRotation(Netcode::ComponentID id, const glm::vec3& rotation);
	void playerJumped(Netcode::ComponentID id);
	void waterHitPlayer(Netcode::ComponentID id);
	void playerDied(Netcode::ComponentID id);
	void playerDisconnect(Netcode::PlayerID playerID);
	void setCandleHeldState(Netcode::ComponentID id, bool b, const glm::vec3& pos = glm::vec3(0, 0, 0));
	void matchEnded();
	void backToLobby();

	void setGameStatePtr(GameState* ptr) { m_gameStatePtr = ptr; }
};
