#pragma once
#include "../BaseComponentSystem.h"

class RendererWrapper;

class WaterCleaningSystem final : public BaseComponentSystem {
public:
	WaterCleaningSystem();
	~WaterCleaningSystem();

	void update(float dt) override;

private:
	RendererWrapper* m_rendererWrapperRef;

	float m_powerUpThreshold;
};
