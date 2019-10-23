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

	void init(unsigned char playerID, GameState* gameStatePtr, NetworkSenderSystem* netSendSysPtr);
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
	unsigned char m_playerID;

	Entity* m_playerEntity = nullptr;
private:
	//void processData(Netcode::MessageType dataType, Netcode::EntityType* entityType, cereal::PortableBinaryInputArchive* ar);

	void createEntity(Netcode::NetworkObjectID id, Netcode::EntityType entityType, const glm::vec3& translation);
	void setEntityTranslation(Netcode::NetworkObjectID id, const glm::vec3& translation);
	void setEntityRotation(Netcode::NetworkObjectID id, const glm::vec3& rotation);
	void setEntityAnimation(Netcode::NetworkObjectID id, int animationStack, float animationTime);
	void playerJumped(Netcode::NetworkObjectID id);
	void projectileSpawned(glm::vec3& pos, glm::vec3 vel);
	void waterHitPlayer(Netcode::NetworkObjectID id, unsigned char SenderId);
	void playerDied(Netcode::NetworkObjectID id, unsigned char shooterID);
	void playerDisconnect(unsigned char id);
	void setCandleHeldState(Netcode::NetworkObjectID id, bool b, const glm::vec3& pos = glm::vec3(0, 0, 0));
	void matchEnded();
	void backToLobby();

	void setGameStatePtr(GameState* ptr) { m_gameStatePtr = ptr; }
};


