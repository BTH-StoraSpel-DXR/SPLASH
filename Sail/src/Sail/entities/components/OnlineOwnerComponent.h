#pragma once

#include "Component.h"
#include "../src/Sail/netcode/NetworkedStructs.h"

class OnlineOwnerComponent : public Component<OnlineOwnerComponent> {
public:
	OnlineOwnerComponent(Netcode::ComponentID netEntityID);
	~OnlineOwnerComponent();

	Netcode::ComponentID netEntityID;
};

