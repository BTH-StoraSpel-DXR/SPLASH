#pragma once
#include "..//BaseComponentSystem.h"

template <typename T>
class MovementPostCollisionSystem final : public BaseComponentSystem {
public:
	MovementPostCollisionSystem();
	~MovementPostCollisionSystem() = default;

	void update(float dt);
private:

};
