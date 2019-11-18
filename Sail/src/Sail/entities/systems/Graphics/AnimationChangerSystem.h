#pragma once

#include "../BaseComponentSystem.h"

class AnimationChangerSystem final : public BaseComponentSystem {
public:
	AnimationChangerSystem();
	~AnimationChangerSystem();

	void update(float dt);

	enum MovementIndex {
	DEFAULT,
	IDLE,
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	IDLEJUMP,
	FORWARDJUMP,
	IDLE_DROP,
	IDLE_THROW,
	SPRINTING,
	//RUNNING_DROP,
	//RUNNING_THROW,
	};
};