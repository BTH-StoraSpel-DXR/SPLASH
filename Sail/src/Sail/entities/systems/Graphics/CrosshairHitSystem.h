#pragma once

#include "../BaseComponentSystem.h"

class Entity;

class CrosshairHitSystem : public BaseComponentSystem {
public:
	CrosshairHitSystem();
	~CrosshairHitSystem();

	void update(float dt);

private:
	
	void alterCrosshair(Entity* e, float dt);

};