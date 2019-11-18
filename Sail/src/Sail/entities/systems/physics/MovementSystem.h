#pragma once
#include "..//BaseComponentSystem.h"

template <typename T>
class MovementSystem final : public BaseComponentSystem {
public:
	MovementSystem();
	~MovementSystem() = default;

	void update(float dt);
};