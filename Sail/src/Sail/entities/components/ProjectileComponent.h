#pragma once
#include "Component.h"
#include "../../../Sail/netcode/NetworkedStructs.h"

class ProjectileComponent : public Component<ProjectileComponent> {
public:
	ProjectileComponent() {}
	ProjectileComponent(float damage, bool isOwnedLocally): m_damage(damage), ownedbyLocalPlayer(isOwnedLocally){}
	~ProjectileComponent() { ; }

	float m_damage = 1.0f;
	bool ownedbyLocalPlayer = false;	// Used to determine hit-reg with network.
	Netcode::ComponentID ownedBy;
	float timeSinceLastDecal = 5000.f;
};
