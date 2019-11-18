#pragma once

#include "Sail/entities/systems/BaseComponentSystem.h"

class Octree;

class CandleThrowingSystem final : public BaseComponentSystem {
public:
	CandleThrowingSystem();
	~CandleThrowingSystem();

	void setOctree(Octree* octree);

	void update(float dt) override;

private:
	void throwCandle(Entity* e);

private:
	Octree* m_octree;

};