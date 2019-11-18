#include "pch.h"
#include "SpectateInputSystem.h"
#include "Sail/entities/components/SpectatorComponent.h"
#include "Sail/entities/components/TransformComponent.h"

#include "Sail/graphics/camera/CameraController.h"
#include "Sail/utils/Utils.h"

#include "../../../api/Input.h"
#include "Sail/KeyBinds.h"


SpectateInputSystem::SpectateInputSystem() : BaseComponentSystem(){
	registerComponent<SpectatorComponent>(true, false, false);
	registerComponent<TransformComponent>(true, true, true);

	// cam variables
	m_yaw = 160.f;
	m_pitch = 0.f;
	m_roll = 0.f;

}

SpectateInputSystem::~SpectateInputSystem() {
	clean();
}

void SpectateInputSystem::fixedUpdate(float dt) {
	processKeyboardInput(dt);
}

void SpectateInputSystem::update(float dt, float alpha) {
	processMouseInput(dt);
	updateCameraPosition(alpha);
}

void SpectateInputSystem::initialize(Camera* cam) {
	if (m_cam == nullptr) {
		m_cam = SAIL_NEW CameraController(cam);
	} 
	else {
		CameraController* tempCam = m_cam;
		Memory::SafeDelete(tempCam);
		m_cam = SAIL_NEW CameraController(cam);
	}
}

void SpectateInputSystem::clean() {
	Memory::SafeDelete(m_cam);
}

void SpectateInputSystem::processKeyboardInput(const float& dt) {

	for (auto e : entities) {

		SpectateMovement playerMovement = getPlayerMovementInput(e);
		glm::vec3 forward = m_cam->getCameraDirection();

		if (playerMovement.forwardMovement != 0.f || playerMovement.rightMovement != 0.f || playerMovement.upMovement != 0.f) {
			forward = glm::normalize(forward);

			// Calculate right vector for player
			glm::vec3 right = glm::cross(glm::vec3(0.f, 1.f, 0.f), forward);
			right = glm::normalize(right);

			glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);

			auto transComp = e->getComponent<TransformComponent>();
			float speed = 10.f;
			glm::vec3 moveDir = (playerMovement.forwardMovement * forward + playerMovement.rightMovement * right + playerMovement.upMovement * up);
			moveDir = glm::normalize(moveDir);
			transComp->translate(moveDir * dt * playerMovement.speedModifier * speed);
		}

	}

}

void SpectateInputSystem::processMouseInput(const float& dt) {

	

	// Toggle cursor capture on right click
	for (auto e : entities) {	
		if (Input::WasMouseButtonJustPressed(KeyBinds::DISABLE_CURSOR)) {
			Input::HideCursor(!Input::IsCursorHidden());
		}
		auto trans = e->getComponent<TransformComponent>();
		auto rots = trans->getRotations();
		m_pitch = (rots.z != 0.f) ? glm::degrees(-rots.z) : m_pitch;
		m_yaw = glm::degrees(-rots.y);

		// Update pitch & yaw if window has focus
		if (Input::IsCursorHidden()) {
			glm::ivec2& mouseDelta = Input::GetMouseDelta();
			m_pitch -= mouseDelta.y * m_lookSensitivityMouse;
			m_yaw -= mouseDelta.x * m_lookSensitivityMouse;
		}

		// Lock pitch to the range -89 - 89
		if (m_pitch >= 89) {
			m_pitch = 89;
		}
		else if (m_pitch <= -89) {
			m_pitch = -89;
		}

		// Lock yaw to the range 0 - 360
		if (m_yaw >= 360) {
			m_yaw -= 360;
		}
		else if (m_yaw <= 0) {
			m_yaw += 360;
		}

		trans->setRotations(0.f, glm::radians(-m_yaw), 0.f);
		
	}

}

void SpectateInputSystem::updateCameraPosition(float alpha) {
	for (auto e : entities) {
		TransformComponent* playerTrans = e->getComponent<TransformComponent>();

		glm::vec3 forwards(
			std::cos(glm::radians(m_pitch)) * std::cos(glm::radians(m_yaw + 90)),
			std::sin(glm::radians(m_pitch)),
			std::cos(glm::radians(m_pitch)) * std::sin(glm::radians(m_yaw + 90))
		);
		forwards = glm::normalize(forwards);

		playerTrans->setRotations(0.f, glm::radians(-m_yaw), 0.f);

		m_cam->setCameraPosition(glm::vec3(playerTrans->getInterpolatedTranslation(alpha) + glm::vec3(0.f, 0.0f * 1.8f, 0.f)));
		m_cam->setCameraDirection(forwards);
	}
}

SpectateMovement SpectateInputSystem::getPlayerMovementInput(Entity* e) {
	SpectateMovement playerMovement;

	if (Input::IsKeyPressed(KeyBinds::MOVE_FORWARD)) { 
		playerMovement.forwardMovement += 1.0f; 
	}
	if (Input::IsKeyPressed(KeyBinds::MOVE_BACKWARD)) {
		playerMovement.forwardMovement -= 1.0f; 
	}
	if (Input::IsKeyPressed(KeyBinds::MOVE_LEFT)) {
		playerMovement.rightMovement -= 1.0f; 
	}
	if (Input::IsKeyPressed(KeyBinds::MOVE_RIGHT)) {
		playerMovement.rightMovement += 1.0f; 
	}
	if (Input::IsKeyPressed(KeyBinds::MOVE_UP)) { 
		playerMovement.upMovement += 1.0f; 
	}
	if (Input::IsKeyPressed(KeyBinds::MOVE_DOWN)) {
		playerMovement.upMovement -= 1.0f; 
	}

	return playerMovement;
}
