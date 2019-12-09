#pragma once


#include "../BaseComponentSystem.h"
#include "Sail/entities/Entity.h"

class CameraController;
class Camera;

struct SpectateMovement {
	float speedModifier = 1.f;
	float forwardMovement = 0.0f;
	float rightMovement = 0.0f;
	float upMovement = 0.0f;
};

class SpectateInputSystem final : public BaseComponentSystem {
public:
	SpectateInputSystem();
	~SpectateInputSystem();

	void fixedUpdate(float dt);
	void update(float dt, float alpha);
	void initialize(Camera* cam);
	void clean();

#ifdef DEVELOPMENT
	unsigned int getByteSize() const override;
#endif

private:
	void processKeyboardInput(const float& dt);
	void processMouseInput(const float& dt);
	void updateCameraPosition(float alpha);
	SpectateMovement getPlayerMovementInput(Entity* e);

	CameraController* m_cam = nullptr;
	float m_yaw, m_pitch, m_roll;
	float m_lookSensitivityMouse = 0.1f;

	float currentMoveSpeed = 1.0f;
};