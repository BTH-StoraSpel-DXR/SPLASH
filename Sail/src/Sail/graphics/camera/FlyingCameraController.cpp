#include "pch.h"
#include "FlyingCameraController.h"
#include "Sail/Application.h"
#include "..//Sail/src/Sail/api/Input.h"
#include "../../KeyCodes.h"
#include "../../MouseButtonCodes.h"

FlyingCameraController::FlyingCameraController(Camera* cam)
	: CameraController(cam)
{
	m_yaw = 90.f;
	m_pitch = 0.f;
	m_roll = 0.f;
}

void FlyingCameraController::setDirection(const glm::vec3 & dir) {
	m_pitch = glm::degrees(glm::asin(dir.y));
	m_yaw = glm::degrees(atan2(dir.x, -dir.z)) - 90.f;
}

void FlyingCameraController::lookAt(const glm::vec3& pos) {
	glm::vec3 dir = pos - getCameraPosition();
	dir = glm::normalize(dir);
	setDirection(dir);
}

void FlyingCameraController::update(float dt) {

	Application* app = Application::getInstance();

	float movementSpeed = dt * 5.f;
	float lookSensitivityMouse = 0.1f;
	float lookSensitivityController = 90.0f * 0.016f;

	// Increase speed if shift or right trigger is pressed
	if (Input::IsKeyPressed(SAIL_KEY_SHIFT))
		movementSpeed *= 5.f;

	//
	// Forwards / backwards motion
	//

	// Gamepad
	//setCameraPosition(getCameraPosition() + getCameraDirection() * gpState.thumbSticks.leftY * movementSpeed);

	// Keyboard
	if (Input::IsKeyPressed(SAIL_KEY_W))
		setCameraPosition(getCameraPosition() + getCameraDirection() * movementSpeed);
	if (Input::IsKeyPressed(SAIL_KEY_S))
		setCameraPosition(getCameraPosition() - getCameraDirection() * movementSpeed);

	//
	// Side to side motion
	//

	glm::vec3 right = glm::cross(getCameraDirection(), glm::vec3(0.f, 1.f, 0.f));
	right = glm::normalize(right);
	// Gamepad
	//setCameraPosition(getCameraPosition() - right * gpState.thumbSticks.leftX * movementSpeed);

	// Keyboard
	if (Input::IsKeyPressed(SAIL_KEY_A))
		setCameraPosition(getCameraPosition() + right * movementSpeed);
	if (Input::IsKeyPressed(SAIL_KEY_D))
		setCameraPosition(getCameraPosition() - right * movementSpeed);

	//
	// Up and down motion
	//

	// Gamepad
	//setCameraPosition(getCameraPosition() + glm::vec3::Up * gpState.buttons.a * movementSpeed);
	//setCameraPosition(getCameraPosition() + glm::vec3::Down * gpState.buttons.x * movementSpeed);

	// Keyboard
	if (Input::IsKeyPressed(SAIL_KEY_SPACE))
		setCameraPosition(getCameraPosition() + glm::vec3(0.f, 1.f, 0.f) * movementSpeed);
	if (Input::IsKeyPressed(SAIL_KEY_CONTROL))
		setCameraPosition(getCameraPosition() + glm::vec3(0.f, -1.f, 0.f) * movementSpeed);

	//
	// Look around motion
	//

	// Gamepad
	//m_pitch += gpState.thumbSticks.rightY * lookSensitivityController;
	//m_yaw -= gpState.thumbSticks.rightX * lookSensitivityController;

	// Mouse input

	// Toggle cursor capture on right click
	if (Input::WasMouseButtonJustPressed(SAIL_MOUSE_RIGHT_BUTTON)) {
		Input::HideCursor(!Input::IsCursorHidden());
	}

	if (Input::IsCursorHidden()) {
		glm::ivec2& mouseDelta = Input::GetMouseDelta();
		m_pitch -= mouseDelta.y * lookSensitivityMouse;
		m_yaw -= mouseDelta.x * lookSensitivityMouse;
	}


	// Lock pitch to the range -89 - 89
	if (m_pitch >= 89)
		m_pitch = 89;
	else if (m_pitch <= -89)
		m_pitch = -89;

	// Lock yaw to the range 0 - 360
	if (m_yaw >= 360)
		m_yaw -= 360;
	else if (m_yaw <= 0)
		m_yaw += 360;

	glm::vec3 forwards(
		std::cos(glm::radians(m_pitch)) * std::cos(glm::radians(m_yaw)),
		std::sin(glm::radians(m_pitch)),
		std::cos(glm::radians(m_pitch)) * std::sin(glm::radians(m_yaw))
	);
	forwards = glm::normalize(forwards);

	setCameraDirection(forwards);

}
