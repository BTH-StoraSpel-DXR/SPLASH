#pragma once
#include "../BaseComponentSystem.h"

class LightSetup;

template <typename T>
class LightSystem final : public BaseComponentSystem {
public:
	LightSystem();
	~LightSystem();

	void prepareFixedUpdate();

	void updateLights(LightSetup* lightSetup, const float alpha);
};
