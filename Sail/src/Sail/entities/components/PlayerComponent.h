#pragma once
#include "Component.h"


// Is currently used as a flag to identify that an entity is a player entity
// Should only be used on the actual players and not on their candles, weapons, etc.
class PlayerComponent : public Component<PlayerComponent> {
public:
	PlayerComponent() {}
	virtual ~PlayerComponent() {}
};