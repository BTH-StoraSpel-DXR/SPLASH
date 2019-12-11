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
#include "../src/Network/NWrapperSingleton.h"
#include "Sail/entities/systems/Gameplay/LevelSystem/LevelSystem.h"
#include "../Sail/src/API/DX12/renderer/DX12RaytracingRenderer.h"

#include "Sail/TimeSettings.h"


// Candle can only be picked up and put down once every 2 seconds
constexpr float CANDLE_TIMER = 2.f;


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
	registerComponent<SprintingComponent>(true, true, false);
	registerComponent<AnimationComponent>(true, true, false);
	registerComponent<NetworkSenderComponent>(false, true, true);
	m_mapPointer = nullptr;

	// cam variables
	m_yaw = 160.f;
	m_pitch = 0.f;
	m_roll = 0.f;

	m_gameDataTracker = &GameDataTracker::getInstance();

	for (int i = 0; i < 5; i++) {
		m_playerPosHolder[i] = { 0,0,0 };
	}
}

GameInputSystem::~GameInputSystem() {
	clean();
}

void GameInputSystem::fixedUpdate(float dt) {
	this->processKeyboardInput(dt);
}

void GameInputSystem::update(float dt, float alpha) {
	this->updateCameraPosition(alpha);
}

void GameInputSystem::initialize(Camera* cam) {
	// If a camera doesn't exist...
	if ( m_cam == nullptr ) {
		// ... Create it
		m_cam = SAIL_NEW CameraController(cam);
	} 
	// If it does exist...
	else {
		// ... Delete it then create it!?!?
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

		//Update candleToggleTimer
		for (auto torch : e->getChildEntities()) {
			if (torch->hasComponent<CandleComponent>()) {
				torch->getComponent<CandleComponent>()->candleToggleTimer += dt;
			}
		}
		// Calculate forward vector for player
		glm::vec3 forward = m_cam->getCameraDirection();

		// Do flying movement if in spectator mode
		if ( e->hasComponent<SpectatorComponent>() ) {
			if (m_killSoundUponDeath) {

				NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
					Netcode::MessageType::RUNNING_STOP_SOUND,
					SAIL_NEW Netcode::MessageRunningStopSound{ e->getComponent<NetworkSenderComponent>()->m_id }
				);
				m_killSoundUponDeath = false;
			}

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
			auto transformComp = e->getComponent<TransformComponent>()->getTranslation();

			m_playerPosHolder[0] = { transformComp.x, 0, transformComp.z};
			m_playerPosHolder[1] = { transformComp.x + DETECTION_STEP_SIZE, 0, transformComp.z + DETECTION_STEP_SIZE };
			m_playerPosHolder[2] = { transformComp.x + DETECTION_STEP_SIZE, 0, transformComp.z - DETECTION_STEP_SIZE };
			m_playerPosHolder[3] = { transformComp.x - DETECTION_STEP_SIZE, 0, transformComp.z + DETECTION_STEP_SIZE };
			m_playerPosHolder[4] = { transformComp.x - DETECTION_STEP_SIZE, 0, transformComp.z - DETECTION_STEP_SIZE };
			
			// Check for nearby water
			for (int i = 0; i < 5; i++) {
				if (m_isOnWaterHolder = Application::getInstance()->getRenderWrapper()->checkIfOnWater(m_playerPosHolder[i])) {
					break;
				}
			}

			// Get player movement inputs
			Movement playerMovement = getPlayerMovementInput(e);

			// Player puts down candle
			toggleCandleCarry(e);

			if ( Input::IsKeyPressed(KeyBinds::LIGHT_CANDLE) ) {
				for ( auto child : e->getChildEntities() ) {
					if ( child->hasComponent<CandleComponent>() ) {
						auto candle = child->getComponent<CandleComponent>();
						if (!candle->isLit) {
							candle->userReignition = true;
						}
					}
				}
			}

			if (collision->onGround) {
				if (m_fallTimer > m_fallThreshold) {
					// Send event to play the sound for the landing (will be sent to ourself too)
					NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
						Netcode::MessageType::PLAYER_LANDED,
						SAIL_NEW Netcode::MessagePlayerLanded{ e->getComponent<NetworkSenderComponent>()->m_id }
					);
				}
				else if (m_fallTimer > 0.0f) {

					m_onGroundTimer = m_onGroundThreshold;
				}
				m_fallTimer = 0.0f;

				if (m_onGroundTimer >= m_onGroundThreshold) {
					m_isPlayingRunningSound = true;
					m_onGroundTimer = m_onGroundThreshold;

				} else {

					m_onGroundTimer += dt;
				}
			}
			// NOTE: Jumping sets m_onGroundTimer to -1.0f to stop running sound right away.
			//		 Needed for when running normally to avoid 'stuttering' effect from playing
			//		 the sound multiple times within a short time span.
			else if (!collision->onGround) {
				if (m_onGroundTimer < 0.0f) {

					m_onGroundTimer = 0.0f;
					m_isPlayingRunningSound = false;
				} else {
					m_fallTimer += dt;
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
			} else if (m_wasSpacePressed){
				m_wasSpacePressed = false;
			}

			forward.y = 0.f;
			forward = glm::normalize(forward);
			// Calculate right vector for player
			glm::vec3 right = glm::cross(glm::vec3(0.f, 1.f, 0.f), forward);
			right = glm::normalize(right);


			// calculate forward vel and sidevel

			movement->relVel.x = glm::dot(movement->velocity, forward);
			movement->relVel.z = glm::dot(movement->velocity, right);
			movement->relVel.y = glm::dot(movement->velocity, glm::vec3(0.f, 0.1f, 0.f));

			m_soundSwitchTimer += dt;

			// Prevent division by zero
			if ( playerMovement.forwardMovement != 0.0f || playerMovement.rightMovement != 0.0f ) {

				//speedLimit->maxSpeed = 1.0f;

				// Calculate total movement
				float acceleration = 70.0f - ( glm::length(movement->velocity) / speedLimit->maxSpeed ) * 20.0f;
				if ( !collision->onGround ) {
					acceleration = acceleration * 0.5f;
					// AUDIO TESTING (turn OFF looping running sound)
					if (!m_isPlayingRunningSound) {
						// If-statement and relevant bools are to avoid sending unnecessary amount of messages/data
						if (!tempStopAll) {
							NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
								Netcode::MessageType::RUNNING_STOP_SOUND,
								SAIL_NEW Netcode::MessageRunningStopSound{ e->getComponent<NetworkSenderComponent>()->m_id }
							);

							tempStopAll = true;

							tempMetal = false;
							tempWaterMetal = false;
							tempTile = false;
							tempWaterTile = false;
							m_soundSwitchTimer = 0.0f;
						}
					}
				}
				// AUDIO TESTING (playing a looping running sound)
				else if (tempStopAll || (m_soundSwitchTimer > m_changeThreshold)) {
					m_soundSwitchTimer = 0.0f;
					// CORRIDOR
					if (m_mapPointer->getAreaType(e->getComponent<TransformComponent>()->getTranslation().x, e->getComponent<TransformComponent>()->getTranslation().z) == 0) {
						
						if (m_isOnWaterHolder && !tempWaterMetal) {
							NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
								Netcode::MessageType::RUNNING_WATER_METAL_START,
								SAIL_NEW Netcode::MessageRunningWaterMetalStart{ e->getComponent<NetworkSenderComponent>()->m_id }
							);

							tempWaterMetal = true;

							tempStopAll = false;
							tempMetal = false;
							tempTile = false;
							tempWaterTile = false;
						}

						// If-statement and relevant bools are to avoid sending unnecessary amount of messages/data
						else if (!m_isOnWaterHolder && !tempMetal) {
							NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
								Netcode::MessageType::RUNNING_METAL_START,
								SAIL_NEW Netcode::MessageRunningMetalStart{ e->getComponent<NetworkSenderComponent>()->m_id }
							);

							tempMetal = true;

							tempStopAll = false;
							tempWaterMetal = false;
							tempTile = false;
							tempWaterTile = false;
						}
					}
					// ROOM
					else /*(AreaType > 0)*/ {
						// If-statement and relevant bools are to avoid sending unnecessary amount of messages/data
						if (m_isOnWaterHolder && !tempWaterTile) {
							NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
								Netcode::MessageType::RUNNING_WATER_TILE_START,
								SAIL_NEW Netcode::MessageRunningWaterTileStart{ e->getComponent<NetworkSenderComponent>()->m_id }
							);

							tempStopAll = false;
							tempMetal = false;
							tempWaterMetal = false;
							tempTile = false;

							tempWaterTile = true;
						}

						else if (!m_isOnWaterHolder && !tempTile) {
							NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
								Netcode::MessageType::RUNNING_TILE_START,
								SAIL_NEW Netcode::MessageRunningTileStart{ e->getComponent<NetworkSenderComponent>()->m_id }
							);

							tempStopAll = false;
							tempMetal = false;
							tempWaterMetal = false;
							tempWaterTile = false;

							tempTile = true;
						}
					}
				} 

				movement->accelerationToAdd =
					glm::normalize(right * playerMovement.rightMovement + forward * playerMovement.forwardMovement * playerMovement.speedModifier)
					* acceleration;



			} else {

				// AUDIO TESTING (turn OFF looping running sound)
				// If-statement and relevant bools are to avoid sending unnecessary amount of messages/data
				if (!tempStopAll) {
					NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
						Netcode::MessageType::RUNNING_STOP_SOUND,
						SAIL_NEW Netcode::MessageRunningStopSound{ e->getComponent<NetworkSenderComponent>()->m_id }
					);

					tempStopAll = true;

					tempMetal = false;
					tempWaterMetal = false;
					tempTile = false;
					tempWaterTile = false;
				}
			}
		}
	}
}

void GameInputSystem::processMouseInput(const float& dt) {
	// Toggle cursor capture on right click
	for (auto e : entities) {

//#ifdef DEVELOPMENT
		if (Input::WasMouseButtonJustPressed(KeyBinds::DISABLE_CURSOR)) {
			Input::HideCursor(!Input::IsCursorHidden());
		}
//#endif
		
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

		// Lock pitch to the range -70 - 55
		if (m_pitch >= 55) {
			m_pitch = 55;
		}
		else if (m_pitch <= -70) {
			m_pitch = -70;
		}

		// Lock yaw to the range 0 - 360
		if (m_yaw >= 360) {
			m_yaw -= 360;
		}
		else if (m_yaw <= 0) {
			m_yaw += 360;
		}

		trans->setRotations(0.f, glm::radians(-m_yaw), 0.f);

		GunComponent* gc = e->getComponent<GunComponent>();

		// Rotate gun
		if (gc) {
			for (auto childE : e->getChildEntities()) {
				if (childE->getName().find("WaterGun") != std::string::npos) {
					TransformComponent* tc = childE->getComponent<TransformComponent>();
					tc->setRotations(glm::radians(-m_pitch),0, 0);
				}
			}
		}

		// Manage the firing of the gun
		if (Input::IsMouseButtonPressed(KeyBinds::SHOOT) && !e->hasComponent<SpectatorComponent>() && !e->getComponent<SprintingComponent>()->sprintedLastFrame) {

			// Check if the player's candle is lit
			bool isLit = true;
			for (auto childE : e->getChildEntities()) {
				if (auto candleC = childE->getComponent<CandleComponent>()) {
					isLit = candleC->isLit;
					break;
				}
			}

			// Only fire if the candle is lit
			if (isLit) {
				if (gc) {
					for (auto childE : e->getChildEntities()) {
						if (childE->getName().find("WaterGun") != std::string::npos) {
							TransformComponent* tc = childE->getComponent<TransformComponent>();
							glm::vec3 gunPosition = glm::vec3(tc->getMatrixWithUpdate()[3]) + m_cam->getCameraDirection() * 0.33f;

							gc->setFiring(gunPosition, m_cam->getCameraDirection());
						}
					}
				}
			} else {
				// Stop firing when the candle is no longer lit
				if (gc->firing) {
					if (auto nsc = e->getComponent<NetworkSenderComponent>()) {
						nsc->addMessageType(Netcode::MessageType::SHOOT_END);
						gc->state = GunState::ENDING;
					}
					gc->firing = false;
				}
			}
		} else {
			if (gc) {
				gc->firing = false;
			}
		}
	}
}


void GameInputSystem::updateCameraPosition(float alpha) {
	for ( auto e : entities ) {
		TransformComponent* playerTrans = e->getComponent<TransformComponent>();
		BoundingBoxComponent* playerBB = e->getComponent<BoundingBoxComponent>();
		AnimationComponent* animation = e->getComponent<AnimationComponent>();

		playerTrans->setRotations(0.f, glm::radians(-m_yaw), 0.f);
		animation->pitch = glm::radians(-m_pitch);

		const glm::vec3 camPos = playerTrans->getMatrixWithUpdate() * glm::vec4(animation->headPositionLocalCurrent, 1.f);

		m_cam->setCameraPosition(camPos);
	}

	const float cosRadPitch = std::cosf(glm::radians(m_pitch));
	const float sinRadPitch = std::sinf(glm::radians(m_pitch));
	const float cosRadYaw = std::cosf(glm::radians(m_yaw + 90));
	const float sinRadYaw = std::sinf(glm::radians(m_yaw + 90));
	
	const glm::vec3 forwards = glm::normalize(glm::vec3(
		cosRadPitch * cosRadYaw,
		sinRadPitch,
		cosRadPitch * sinRadYaw));
	
	m_cam->setCameraDirection(forwards);
}

CameraController* GameInputSystem::getCamera() const {
	return m_cam;
}

#ifdef DEVELOPMENT
unsigned int GameInputSystem::getByteSize() const {
	unsigned int size = BaseComponentSystem::getByteSize() + sizeof(*this);
	size += sizeof(CameraController);
	return size;
}
#endif

void GameInputSystem::toggleCandleCarry(Entity* entity) {
	for (auto torch : entity->getChildEntities()) {
		if (torch->hasComponent<CandleComponent>()) {
			if (torch->getComponent<CandleComponent>()->candleToggleTimer < CANDLE_TIMER) {
				// No need to do anything since it's to soon since last action
				return;
			}
		}
	}

	for (int i = 0; i < entity->getChildEntities().size(); i++) {
		auto torchE = entity->getChildEntities()[i];
		if (torchE->hasComponent<CandleComponent>()) {
			auto candleComp = torchE->getComponent<CandleComponent>();
			if (candleComp->isLit) {
				bool chargeHeld = Input::IsKeyPressed(KeyBinds::THROW_CHARGE);

				auto throwingComp = entity->getComponent<ThrowingComponent>();
				if (!throwingComp->isThrowing) {
					if (chargeHeld) {
						if (candleComp->isCarried && torchE->getComponent<TransformComponent>()->getParent()) {
							// Torch is carried, get to charging the throw
							throwingComp->isCharging = true;
							if (throwingComp->chargeTime >= throwingComp->chargeToThrowThreshold) {
								// We want to throw the torch
								throwingComp->isCharging = false;
								//candleComp->candleToggleTimer = 0.f;
							}
						} else {
							// Torch isn't carried so try to pick it up
							candleComp->isCarried = true;
							candleComp->candleToggleTimer = 0.f;
						}
					} else if (candleComp->isCarried && throwingComp->wasChargingLastFrame) {
						// We want to throw the torch
						throwingComp->isCharging = false;
						//candleComp->candleToggleTimer = 0.f;
					}
				}
			}


			return;
		}
	}	
}

Movement GameInputSystem::getPlayerMovementInput(Entity* e) {
	Movement playerMovement;

	auto sprintComp = e->getComponent<SprintingComponent>();
	sprintComp->doSprint = false;

	if (Input::IsKeyPressed(KeyBinds::MOVE_FORWARD)) { playerMovement.forwardMovement += 1.0f; }
	if (Input::IsKeyPressed(KeyBinds::MOVE_BACKWARD)) { playerMovement.forwardMovement -= 1.0f; }
	if (Input::IsKeyPressed(KeyBinds::MOVE_LEFT)) { playerMovement.rightMovement -= 1.0f; }
	if (Input::IsKeyPressed(KeyBinds::MOVE_RIGHT)) { playerMovement.rightMovement += 1.0f; }
	if (Input::IsKeyPressed(KeyBinds::MOVE_UP)) { playerMovement.upMovement += 1.0f; }
	if (Input::IsKeyPressed(KeyBinds::MOVE_DOWN)) { playerMovement.upMovement -= 1.0f; }

	auto animC = e->getComponent<AnimationComponent>();
	auto throwC = e->getComponent<ThrowingComponent>();
	if (animC->animationIndex == 9/*IDLE_THROW*/ || animC->animationIndex == 11/*RUNNING_THROW*/ || throwC->chargeTime > 0.f) {
		return playerMovement;
	}

	if (Input::IsKeyPressed(KeyBinds::SPRINT)) {
		if (!e->hasComponent<SpectatorComponent>()) {
			if (sprintComp->canSprint && Input::IsKeyPressed(KeyBinds::MOVE_FORWARD)) {
				sprintComp->doSprint = true;
				playerMovement.speedModifier = sprintComp->sprintSpeedModifier;
			}
		// For spectator
		} else {
			playerMovement.speedModifier = sprintComp->sprintSpeedModifier;
		}
	}

	return playerMovement;
}
