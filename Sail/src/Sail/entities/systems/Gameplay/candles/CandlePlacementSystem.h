#pragma once

#include "Sail/entities/systems/BaseComponentSystem.h"
#include "Sail/events/EventReceiver.h"

class Octree;

class CandlePlacementSystem final : public BaseComponentSystem, public EventReceiver {
public:
	CandlePlacementSystem();
	~CandlePlacementSystem();

	void update(float dt) override;

private:
	void toggleCandlePlacement(Entity* e);
	bool onEvent(const Event& event) override;

};