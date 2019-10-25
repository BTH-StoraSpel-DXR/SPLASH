#include "pch.h"
#include "GameInputSystem.h"
#include "Sail/graphics/camera/CameraController.h"
#include "Sail/entities/components/Components.h"
#include "../../../api/Input.h"
#include "Sail/KeyBinds.h"
#include "Sail/Application.h"
#include "Sail/utils/GameDataTracker.h"
#include "../../ECS.h"
#include "../physics/UpdateBoundingBoxSystem.h"
#include "Sail/entities/components/LocalOwnerComponent.h"
#include "Sail/entities/components/AudioComponent.h"
#include "../src/Network/NWrapperSingleton.h"

GameInputSystem::GameInputSystem() : BaseComponentSystem() {
	registerComponent<LocalOwnerComponent>(true, true, false);
	registerComponent<MovementComponent>(true, true, true);
	registerComponent<SpeedLimitComponent>(true, true, false);
	registerComponent<CollisionComponent>(true, true, false);
	registerComponent<AudioComponent>(true, true, true);
	registerComponent<BoundingBoxComponent>(true, true, false);
	registerComponent<TransformComponent>(true, true, true);
	registerComponent<CandleComponent>(false, true, true);
	registerComponent<GunComponent>(false, true, true);

	// cam variables
	m_yaw = 160.f;
	m_pitch = 0.f;
	m_roll = 0.f;

	m_gameDataTracker = &GameDataTracker::getInstance();
}

GameInputSystem::~GameInputSystem() {
	clean();
}


void GameInputSystem::update(float dt, float alpha) {
	this->processKeyboardInput(dt);
	this->processMouseInput(dt);
	this->updateCameraPosition(alpha);
}

void GameInputSystem::initialize(Camera* cam) {
	if ( m_cam == nullptr ) {
		m_cam = SAIL_NEW CameraController(cam);
	} else {
		CameraController* tempCam = m_cam;
		Memory::SafeDelete(tempCam);
		m_cam = SAIL_NEW CameraController(cam);
	}
}

void GameInputSystem::clean() {
	Memory::SafeDelete(m_cam);
}

void GameInputSystem::stop() {
	clean();
}

void GameInputSystem::processKeyboardInput(const float& dt) {
	for ( auto e : entities ) {
		// Get player movement inputs
		Movement playerMovement = getPlayerMovementInput(e);

		// Calculate forward vector for player
		glm::vec3 forward = m_cam->getCameraDirection();

		// Do flying movement if in spectator mode
		if ( e->hasComponent<SpectatorComponent>() ) {
			if ( playerMovement.forwardMovement != 0.f || playerMovement.rightMovement != 0.f || playerMovement.upMovement != 0.f ) {
 				forward = glm::normalize(forward);

				// Calculate right vector for player
				glm::vec3 right = glm::cross(glm::vec3(0.f, 1.f, 0.f), forward);
				right = glm::normalize(right);

				glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);

				auto transComp = e->getComponent<TransformComponent>();
				float speed = 5.f;
				glm::vec3 moveDir = ( playerMovement.forwardMovement * forward + playerMovement.rightMovement * right + playerMovement.upMovement * up );
				moveDir = glm::normalize(moveDir);
				transComp->translate(moveDir * dt * playerMovement.speedModifier * 10.f);
			}

			// Else do normal movement
		} else {
			auto collision = e->getComponent<CollisionComponent>();
			auto movement = e->getComponent<MovementComponent>();
			auto speedLimit = e->getComponent<SpeedLimitComponent>();
			auto audioComp = e->getComponent<AudioComponent>();


			// Get player movement inputs
			Movement playerMovement = getPlayerMovementInput(e);

			// Player puts down candle
			if ( Input::WasKeyJustPressed(KeyBinds::putDownCandle) ) {

				putDownCandle(e);
			}

			if ( Input::WasKeyJustPressed(KeyBinds::lightCandle) ) {

				for ( auto child : e->getChildEntities() ) {

					if ( child->hasComponent<CandleComponent>() ) {

						child->getComponent<CandleComponent>()->activate();
					}
				}
			}

			if (collision->onGround) {

				m_isPlayingRunningSound = true;

				if (m_onGroundTimer > m_onGroundThreshold) {

					m_onGroundTimer = m_onGroundThreshold;
				} else {

					m_onGroundTimer += dt;
				}
			}

			else if (!collision->onGround) {

				if (m_onGroundTimer < 0.0f) {

					m_onGroundTimer = 0.0f;
					m_isPlayingRunningSound = false;
				} else {

					m_onGroundTimer -= dt;
				}
			}

			if ( playerMovement.upMovement == 1.0f ) {

				if (!m_wasSpacePressed && collision->onGround) {

					movement->velocity.y = 5.0f;
					// AUDIO TESTING - JUMPING
					m_onGroundTimer = -1.0f; // To stop walking sound immediately when jumping
					m_gameDataTracker->logJump();

					// Send event to play the sound for the jump (will be sent to ourself too)
					NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
						Netcode::MessageType::PLAYER_JUMPED,
						SAIL_NEW Netcode::MessagePlayerJumped{ e->getComponent<NetworkSenderComponent>()->m_id }
					);
				}
				m_wasSpacePressed = true;
			} else {
				m_wasSpacePressed = false;
			}

			forward.y = 0.f;
			forward = glm::normalize(forward);

			// Calculate right vector for player
			glm::vec3 right = glm::cross(glm::vec3(0.f, 1.f, 0.f), forward);
			right = glm::normalize(right);

			// Prevent division by zero
			if ( playerMovement.forwardMovement != 0.0f || playerMovement.rightMovement != 0.0f ) {

				// Calculate total movement
				float acceleration = 70.0f - ( glm::length(movement->velocity) / speedLimit->maxSpeed ) * 20.0f;
				if ( !collision->onGround ) {

					acceleration = acceleration * 0.5f;
					// AUDIO TESTING (turn OFF looping running sound)
					if (!m_isPlayingRunningSound) {
						audioComp->m_sounds[Audio::SoundType::RUN_METAL].isPlaying = false;
					}
				}
				// AUDIO TESTING (playing a looping running sound)
				else if ( m_runSoundTimer > m_onGroundThreshold) {

					audioComp->m_sounds[Audio::SoundType::RUN_METAL].isPlaying = true;
				} else {

					m_runSoundTimer += dt;
				}

				movement->accelerationToAdd =
					glm::normalize(right * playerMovement.rightMovement + forward * playerMovement.forwardMovement)
					* acceleration;
			} else {

				// AUDIO TESTING (turn OFF looping running sound)
				audioComp->m_sounds[Audio::SoundType::RUN_METAL].isPlaying = false;
				m_runSoundTimer = 0.0f;
			}
		}
	}
}

void GameInputSystem::processMouseInput(const float& dt) {
	// Toggle cursor capture on right click
	for (auto e : entities) {

#ifdef DEVELOPMENT
		if (Input::WasMouseButtonJustPressed(KeyBinds::disableCursor)) {
			Input::HideCursor(!Input::IsCursorHidden());
		}
#endif

		if (!e->hasComponent<SpectatorComponent>() && Input::IsMouseButtonPressed(KeyBinds::shoot)) {
			glm::vec3 camRight = glm::cross(m_cam->getCameraUp(), m_cam->getCameraDirection());
			glm::vec3 gunPosition = m_cam->getCameraPosition() + (m_cam->getCameraDirection() + camRight - m_cam->getCameraUp());
			e->getComponent<GunComponent>()->setFiring(gunPosition, m_cam->getCameraDirection());
		}
		else {
			if (e->hasComponent<GunComponent>()) {
				e->getComponent<GunComponent>()->firing = false;
			}
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


void GameInputSystem::updateCameraPosition(float alpha) {
	for ( auto e : entities ) {
		TransformComponent* playerTrans = e->getComponent<TransformComponent>();
		BoundingBoxComponent* playerBB = e->getComponent<BoundingBoxComponent>();

		glm::vec3 forwards(
			std::cos(glm::radians(m_pitch)) * std::cos(glm::radians(m_yaw)),
			std::sin(glm::radians(m_pitch)),
			std::cos(glm::radians(m_pitch)) * std::sin(glm::radians(m_yaw))
		);
		forwards = glm::normalize(forwards);

		m_cam->setCameraPosition(glm::vec3(playerTrans->getInterpolatedTranslation(alpha) + glm::vec3(0.f, playerBB->getBoundingBox()->getHalfSize().y * 1.8f, 0.f)));
		m_cam->setCameraDirection(forwards);
	}
}

CameraController* GameInputSystem::getCamera() const {
	return m_cam;
}

void GameInputSystem::putDownCandle(Entity* e) {
	for ( int i = 0; i < e->getChildEntities().size(); i++ ) {
		auto candleE = e->getChildEntities()[i];
		if ( candleE->hasComponent<CandleComponent>() ) {
			auto candleComp = candleE->getComponent<CandleComponent>();
			candleComp->setCarried(!candleComp->isCarried());
			return;
		}
	}
}

Movement GameInputSystem::getPlayerMovementInput(Entity* e) {
	Movement playerMovement;

	if ( Input::IsKeyPressed(KeyBinds::sprint) ) { playerMovement.speedModifier = m_runSpeed; }

	if ( Input::IsKeyPressed(KeyBinds::moveForward) ) { playerMovement.forwardMovement += 1.0f; }
	if ( Input::IsKeyPressed(KeyBinds::moveBackward) ) { playerMovement.forwardMovement -= 1.0f; }
	if ( Input::IsKeyPressed(KeyBinds::moveLeft) ) { playerMovement.rightMovement -= 1.0f; }
	if ( Input::IsKeyPressed(KeyBinds::moveRight) ) { playerMovement.rightMovement += 1.0f; }
	if ( Input::IsKeyPressed(KeyBinds::moveUp) ) { playerMovement.upMovement += 1.0f; }
	if ( Input::IsKeyPressed(KeyBinds::moveDown) ) { playerMovement.upMovement -= 1.0f; }

	return playerMovement;
}