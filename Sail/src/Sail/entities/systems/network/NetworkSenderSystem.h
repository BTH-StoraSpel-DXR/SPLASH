#pragma once

#include "../BaseComponentSystem.h"
#include "Sail/../../libraries/cereal/archives/portable_binary.hpp"
#include "Sail/netcode/NetworkedStructs.h"
// "Sail/../../libraries/cereal/archives/portable_binary.hpp"

class Entity;
class MessageType;
struct NetworkSenderEvent;

class NetworkSenderSystem : public BaseComponentSystem {
public:
	NetworkSenderSystem();
	~NetworkSenderSystem();
	void update();

	const void queueEvent(NetworkSenderEvent* event);
	
	virtual void stop();

	void addEntityToListONLYFORNETWORKRECIEVER(Entity* e);
	void initWithPlayerID(unsigned char playerID);
	void initPlayerEntity(Entity* pPlayerEntity);

	void pushDataToBuffer(std::string data);


private:
	unsigned char m_playerID;
	Entity* m_playerEntity = nullptr;

	void handleEvent(Netcode::MessageType& messageType, Entity* e, cereal::PortableBinaryOutputArchive* ar);
	void handleEvent(NetworkSenderEvent* event, cereal::PortableBinaryOutputArchive* ar);
	std::queue<NetworkSenderEvent*> eventQueue;



	// The host will copy incoming packages to this queue and then send them all out in update()
	// This will automatically forward all packets client send to the host to all clients who are connected to host.



	// How a packet (a serialized data string) will travel.
	// NSS = NetworkSenderSystem
	// NRS = NetworkReceiverSystem


	//             ,->Host(NRS) 
	//            /            ,-> Client1(NRS) (will ignore eventually)
	// Client1(NSS)-->Host(NSS)--> Client2(NRS)
	//                         `-> Client3(NRS)


	std::queue<std::string> m_HOSTONLY_dataToForward;
	std::mutex m_forwardBufferLock;

	//void archiveData(Netcode::MessageType* type, Entity* e, cereal::PortableBinaryOutputArchive* ar);
};