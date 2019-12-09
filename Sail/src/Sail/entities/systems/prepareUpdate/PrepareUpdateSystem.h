#pragma once
#include "..//BaseComponentSystem.h"

// Used to prepare all transform components at the beginning of each CPU update
class PrepareUpdateSystem : public BaseComponentSystem {
public:
	PrepareUpdateSystem();
	~PrepareUpdateSystem();
	void fixedUpdate();
	void update();
};