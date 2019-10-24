#pragma once

#include "../BaseComponentSystem.h"
#include "Sail/../../libraries/cereal/archives/portable_binary.hpp"
#include "Sail/netcode/NetworkedStructs.h"

#include "Sail/netcode/ArchiveTypes.h"
#include "Sail/netcode/NetcodeTypes.h"



class Entity;
class MessageType;
class NetworkReceiverSystem;
struct NetworkSenderEvent;

class NetworkSenderSystem : public BaseComponentSystem {
public:
	NetworkSenderSystem();
	~NetworkSenderSystem();

	void update();
	virtual void stop() override;

	void init(Netcode::PlayerID playerID, NetworkReceiverSystem* receiverSystem);
	
	void queueEvent(NetworkSenderEvent* event);
	void pushDataToBuffer(std::string data);
	
	// TODO: Is this used?
	const std::vector<Entity*>& getEntities() const;

	void addEntityToListONLYFORNETWORKRECIEVER(Entity* e);
private:
	void writeMessageToArchive(Netcode::MessageType& messageType, Entity* e, Netcode::OutArchive& ar);
	void writeEventToArchive(NetworkSenderEvent* event, Netcode::OutArchive& ar);
	


private:
	Netcode::PlayerID m_playerID;
	std::queue<NetworkSenderEvent*> m_eventQueue;
	std::atomic<size_t> m_nrOfEventsToSendToSelf = 0; // atomic is probably not needed

	NetworkReceiverSystem* m_receiverSystem = nullptr;

	/*
	 * The host will copy incoming packages to this queue and then send them all out in update()
	 * This will automatically forward all packets client send to the host to all clients who are connected to host.
	 *
	 * How a packet (a serialized data string) will travel.
	 * NSS = NetworkSenderSystem
	 * NRS = NetworkReceiverSystem
	 *             ,->Host(NRS)
	 *            /            ,-> Client1(NRS) (will ignore eventually)
	 * Client1(NSS)-->Host(NSS)--> Client2(NRS)
	 *                         `-> Client3(NRS)
	 **/
	std::queue<std::string> m_HOSTONLY_dataToForward;
	std::mutex m_forwardBufferLock;
};
