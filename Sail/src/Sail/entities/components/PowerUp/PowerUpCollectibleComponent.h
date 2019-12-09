#pragma once
#pragma once
#pragma once

#include "../Component.h"
#include "Sail/entities/Entity.h"

class PowerUpCollectibleComponent : public Component<PowerUpCollectibleComponent> {
public:

public:
	PowerUpCollectibleComponent();
	~PowerUpCollectibleComponent();

#ifdef DEVELOPMENT
	const unsigned int getByteSize() const override {
		/* TODO: Fix component size */
		return sizeof(*this);
	}
	void imguiRender(Entity** selected);
#endif

public:


	int powerUp;
	float powerUpDuration;

	float respawnTime; // positive for respawn, negative for single use
	float time;

};