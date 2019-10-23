#pragma once
#include "Component.h"
#include "../src/Sail/netcode/NetworkedStructs.h"

class LocalOwnerComponent : public Component<LocalOwnerComponent> {
public:
	LocalOwnerComponent() { ; }
	LocalOwnerComponent(Netcode::ComponentID netEntityId_) : netEntityID(netEntityId_) {	}
	~LocalOwnerComponent() { }

	Netcode::ComponentID netEntityID;
};