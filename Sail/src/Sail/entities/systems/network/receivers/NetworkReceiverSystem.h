#pragma once

#include "../../BaseComponentSystem.h"
#include "Sail/netcode/NetworkedStructs.h"

class GameState;
class NetworkSenderSystem;

class NetworkReceiverSystem : public BaseComponentSystem {
public:
	NetworkReceiverSystem();
	~NetworkReceiverSystem();

	virtual void handleIncomingData(std::string data) = 0;
	void pushDataToBuffer(std::string);

	void init(Netcode::PlayerID playerID, GameState* gameStatePtr, NetworkSenderSystem* netSendSysPtr);


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
private:
	void createEntity(Netcode::ComponentID id, Netcode::EntityType entityType, const glm::vec3& translation);
	void setEntityTranslation(Netcode::ComponentID id, const glm::vec3& translation);
	void setEntityRotation(Netcode::ComponentID id, const glm::vec3& rotation);
	void setEntityAnimation(Netcode::ComponentID id, int animationStack, float animationTime);
	void playerJumped(Netcode::ComponentID id);
	void waterHitPlayer(Netcode::ComponentID id, Netcode::PlayerID SenderId);
	void projectileSpawned(glm::vec3& pos, glm::vec3 vel);
	void playerDied(Netcode::ComponentID id, Netcode::PlayerID shooterID);
	void playerDisconnect(Netcode::PlayerID playerID);
	void setCandleHeldState(Netcode::ComponentID id, bool b, const glm::vec3& pos = glm::vec3(0, 0, 0));
	//void processData(Netcode::MessageType dataType, Netcode::EntityType* entityType, cereal::PortableBinaryInputArchive* ar);

	void matchEnded();
	void backToLobby();

	void setGameStatePtr(GameState* ptr) { m_gameStatePtr = ptr; }
};
