#include "pch.h"
#include "CameraController.h"
#include "Camera.h"

CameraController::CameraController(Camera* cam) : m_cam(cam) {
}

void CameraController::update(float dt) {
}

void CameraController::setCameralookAt(const glm::vec3& pos) {
	m_cam->lookAt(pos);
}
void CameraController::setCameraDirection(const glm::vec3& direction) {
	m_cam->setDirection(direction);
}
void CameraController::setCameraPosition(const glm::vec3& pos) {
	m_cam->setPosition(pos);
}
const glm::vec3& CameraController::getCameraDirection() {
	return m_cam->m_direction;
}
const glm::vec3& CameraController::getCameraPosition() {
	return m_cam->m_pos;
}
const glm::vec3& CameraController::getCameraUp() {
	return m_cam->getUp();
}
const Camera* CameraController::getCamera() {
	return m_cam;
}