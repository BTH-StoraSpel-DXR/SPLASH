#pragma once
#include "Component.h"
#include "Sail/netcode/NetworkedStructs.h"
#include <vector>


class NetworkSenderComponent : public Component<NetworkSenderComponent> {
public:
	/*
	  type will decide which components are modified for the entity this component belongs to
	  The ownerID is the ID of the player and is used to create a unique ID for the network component
	
	  The first byte of the object ID is set to the ownerID and the rest to the unique ID generated by the player
	*/
	NetworkSenderComponent(Netcode::MessageType msgType, Netcode::EntityType entityType, Netcode::PlayerID ownerID) :
		m_id(Netcode::generateUniqueComponentID(ownerID)), m_entityType(entityType), m_dataTypes{msgType} 
	{}

	/*
	  This function should only be used by the host to create pass-through sender components.
	*/
	NetworkSenderComponent(Netcode::MessageType msgType, Netcode::EntityType entityType, Netcode::ComponentID objectID) :
		m_id(objectID), m_entityType(entityType), m_dataTypes{msgType}
	{}

	~NetworkSenderComponent() {}

	// Removes all message types so that this entity will no longer send any information
	void removeAllMessageTypes() {
		m_dataTypes.clear();
	}

	void addMessageType(Netcode::MessageType type) { 
		m_dataTypes.push_back(type); 
	}

	void removeMessageType(Netcode::MessageType type) {
		// Search through dataType vector
		for (int i = 0; i < m_dataTypes.size(); i++) {
			// If found...
			if (m_dataTypes.at(i) == type) {
				// ... Then delete
				m_dataTypes.erase(m_dataTypes.begin() + i);
			}
		}

		// Then, if size is 0, queue a deletion of this component.
		
	}

	bool messageTypeAlreadyExists(Netcode::MessageType type) {
		for (auto& t : m_dataTypes)	{
			if (t == type) {
				return true;
			}
		}
		return false;
	}

	Netcode::ComponentID m_id;
	Netcode::EntityType m_entityType;

	std::vector<Netcode::MessageType> m_dataTypes;
};
