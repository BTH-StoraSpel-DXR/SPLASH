#pragma once
#include "..//BaseComponentSystem.h"

template <typename T>
class ModelSubmitSystem final : public BaseComponentSystem {
public:
	ModelSubmitSystem();
	~ModelSubmitSystem() = default;

	void submitAll(const float alpha, float dt);


	bool raisingVats = false;
	float timePassed = 0.0f;
	float timeToRaise = 10.0f;
	float yRoof = 0.0f;
	float yFloor = -4.0f;

	void raise(Entity* e);

	void raiseVats();
	void lowerVats();
	void stopVats();
};