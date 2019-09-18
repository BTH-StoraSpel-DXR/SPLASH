#pragma once
#include <glm/vec3.hpp>

class Camera;

class CameraController {
public:
	CameraController(Camera* cam);

	virtual void update(float dt);

	void setCameralookAt(const glm::vec3& pos);
	void setCameraDirection(const glm::vec3& direction);
	void setCameraPosition(const glm::vec3& pos);
	const glm::vec3& getCameraDirection();
	const glm::vec3& getCameraPosition();
	const glm::vec3& getCameraUp();
	const Camera* getCamera();

private:
	Camera* m_cam;

};
