#pragma once
#include "Component.h"
#include "Sail/netcode/NetworkedStructs.h"


class NetworkReceiverComponent : public Component<NetworkReceiverComponent> {
public:
	/*
	  New receiver components are only created when the application receives NetcodeData with a type that should be
	  continuously updated over the network and with a unique ID that they don't have a NetworkReceiverComponent for yet.
	*/
	NetworkReceiverComponent(Netcode::ComponentID id, Netcode::EntityType type) : m_id(id), m_entityType(type) {}
	~NetworkReceiverComponent() {}

	Netcode::ComponentID m_id; // Same as the ID used by whoever is sending the information to this entity
	Netcode::EntityType m_entityType;
};
