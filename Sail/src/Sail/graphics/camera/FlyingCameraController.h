#pragma once

#include "CameraController.h"
#include <glm/vec3.hpp>

class FlyingCameraController : public CameraController {
public:

	FlyingCameraController(Camera* cam);

	void setDirection(const glm::vec3& dir);
	void lookAt(const glm::vec3& pos);

	virtual void update(float dt);

private:
	float m_yaw, m_pitch, m_roll;
};
