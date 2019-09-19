#include "pch.h"
#include "PlayerController.h"
#include "Sail/api/Input.h"
#include "Sail.h"

PlayerController::PlayerController(Camera* cam, Scene* scene) {
	m_cam = SAIL_NEW CameraController(cam);
	m_scene = scene;
	m_player = ECS::Instance()->createEntity("player_entity");
	
	//m_player->addComponent<MovementComponent>(/*initialSpeed*/ 0.f, /*initialDirection*/ m_cam->getCameraDirection());
	m_player->addComponent<TransformComponent>(m_cam->getCameraPosition());

	m_player->getComponent<TransformComponent>()->setTranslation(glm::vec3(0.0f, 3.f, 0.f));

	m_yaw = 90.f;
	m_pitch = 0.f;
	m_roll = 0.f;
}

PlayerController::~PlayerController() {
	delete m_cam;
	m_projectiles.clear();
}

void PlayerController::update(float dt) {
	float speedModifier = 1.f;

	//float forwardM = 0.f, backM = 0.f, leftM = 0.f, rightM = 0.f, upM = 0.f, downM = 0.f;

	float forwardMovement = 0.0f;
	float rightMovement = 0.0f;
	float upMovement = 0.0f;

	PhysicsComponent* physicsComp = m_player->getComponent<PhysicsComponent>();
	//MovementComponent* playerMovComp = m_player->getComponent<MovementComponent>();

	// Increase speed if shift or right trigger is pressed
	if ( Input::IsKeyPressed(SAIL_KEY_SHIFT) ) {
		speedModifier = 5.f;
	}

	//
	// Forwards / backwards motion
	//

	// Keyboard
	if ( Input::IsKeyPressed(SAIL_KEY_W) ) {
		//forwardM = 1.0f;
		forwardMovement += 1.0f;
	}

	if ( Input::IsKeyPressed(SAIL_KEY_S) ) {
		forwardMovement -= 1.0f;
		/*if ( forwardM == 0.f ) {
			backM = 1.0f;
		}
		else {
			forwardM = 0.f;
			backM = 0.f;
		}*/
	}

	//
	// Side to side motion
	//

	// Keyboard
	if ( Input::IsKeyPressed(SAIL_KEY_A) ) {
		rightMovement -= 1.0f;
		//leftM = 1.0f;
	}
	if ( Input::IsKeyPressed(SAIL_KEY_D) ) {
		rightMovement += 1.0f;
		/*rightM = 1.0f;
		if ( leftM == 0.f ) {
			rightM = 1.0f;
		}
		else {
			rightM = 0.f;
			leftM = 0.f;
		}*/
	}

	//
	// Up and down motion
	//

	// Keyboard
	if ( Input::IsKeyPressed(SAIL_KEY_SPACE) ) {
		upMovement += 1.0f;
		//upM = 1.0f;
	}
	if ( Input::IsKeyPressed(SAIL_KEY_CONTROL) ) {
		upMovement -= 1.0f;
		/*downM = 1.0f;
		if ( upM == 0.f ) {
			downM = 1.0f;
		}
		else {
			upM = 0.f;
			downM = 0.f;
		}*/
	}

	//
	// Look around motion
	//

	// Mouse input

	// Toggle cursor capture on right click
	if ( Input::WasMouseButtonJustPressed(SAIL_MOUSE_RIGHT_BUTTON) ) {
		Input::HideCursor(!Input::IsCursorHidden());
	}

	if ( Input::IsCursorHidden() ) {
		glm::ivec2& mouseDelta = Input::GetMouseDelta();
		m_pitch -= mouseDelta.y * m_lookSensitivityMouse;
		m_yaw -= mouseDelta.x * m_lookSensitivityMouse;
	}

	// Shoot gun
	if (Input::IsMouseButtonPressed(0)) {
		if (m_projectileSpawnCounter == 0.f) {

			// Create projectile entity and attaching components
			auto e = ECS::Instance()->createEntity("new cube");
			e->addComponent<ModelComponent>(m_projectileModel);
			glm::vec3 camRight = glm::cross(m_cam->getCameraUp(), m_cam->getCameraDirection());
			e->addComponent<TransformComponent>(m_cam->getCameraPosition() + (m_cam->getCameraDirection() + camRight - m_cam->getCameraUp()), glm::vec3(0.f), glm::vec3(0.1f));
			e->addComponent<PhysicsComponent>();
			e->getComponent<PhysicsComponent>()->velocity = m_cam->getCameraDirection() * 10.f;
			e->getComponent<PhysicsComponent>()->acceleration = glm::vec3(0.f, -10.f, 0.f);

			// Adding projectile to projectile vector to keep track of current projectiles
			Projectile proj;
			proj.projectile = e;
			m_projectiles.push_back(proj);

			// Add entity to scene for rendering, will most likely be changed once scene system is created
			m_scene->addEntity(e);

			m_projectileSpawnCounter += dt;
		}
		else {
			m_projectileSpawnCounter += dt;
			if (m_projectileSpawnCounter > 0.05f) {
				m_projectileSpawnCounter = 0.f;
			}
		}
	}
	else {
		m_projectileSpawnCounter = 0.f;
	}
	// Lock pitch to the range -89 - 89
	if ( m_pitch >= 89 ) {
		m_pitch = 89;
	}
	else if ( m_pitch <= -89 ) {
		m_pitch = -89;
	}

	// Lock yaw to the range 0 - 360
	if ( m_yaw >= 360 ) {
		m_yaw -= 360;
	}
	else if ( m_yaw <= 0 ) {
		m_yaw += 360;
	}

	glm::vec3 forwards(
		std::cos(glm::radians(m_pitch)) * std::cos(glm::radians(m_yaw)),
		std::sin(glm::radians(m_pitch)),
		std::cos(glm::radians(m_pitch)) * std::sin(glm::radians(m_yaw))
	);
	forwards = glm::normalize(forwards);

	glm::vec3 forward = m_cam->getCameraDirection();
	forward.y = 0.f;
	forward = glm::normalize(forward);
	
	glm::vec3 right = glm::cross(glm::vec3(0.f, 1.f, 0.f), forward);
	right = glm::normalize(right);


	// Prevent division by zero
	if (forwardMovement != 0.0f || rightMovement != 0.0f || upMovement != 0.0f) {

		// Calculate total movement
		physicsComp->velocity =
			glm::normalize(right * rightMovement + forward * forwardMovement + glm::vec3(0.0f, 1.0f, 0.0f) * upMovement)
			* (m_movementSpeed * speedModifier);
	}
	else {
		physicsComp->velocity = glm::vec3(0.0f, 0.0f, 0.0f);
	}


	//float totM = forwardM + backM + rightM + leftM;// +upM + downM;
	//if ( totM != 0.f ) {
	//	glm::vec3 dir = ( forward * forwardM ) - ( forward * backM ) + ( right * rightM ) - ( right * leftM );
	//		///*Only for flying*/// + ( m_cam->getCameraUp() * upM ) - ( m_cam->getCameraUp() * downM );
	//
	//	physicsComp->velocity = glm::normalize(dir) * (m_movementSpeed * speedModifier);
	//	/*playerMovComp->setSpeed(m_movementSpeed* speedModifier);
	//	playerMovComp->setDirection(glm::normalize(dir));*/
	//}
	//else {
	//
	//}

	// Update for all projectiles
	for (int i = 0; i < m_projectiles.size(); i++) {
		m_projectiles[i].lifeTime += dt;
		if (m_projectiles[i].lifeTime > 2.f) {
			ECS::Instance()->destroyEntity(m_projectiles[i].projectile);
			m_projectiles.erase(m_projectiles.begin() + i);
			i--;
		}
	}


	TransformComponent* playerTrans = m_player->getComponent<TransformComponent>();
	m_cam->setCameraPosition(playerTrans->getTranslation());
	// TODO: Replace with transform rotation/direction
	/*Logger::Warning("totM: " + std::to_string(totM) + 
					" fowards: " + std::to_string(forwards.x) + 
					" " + std::to_string(forwards.y)
					+ " " + std::to_string(forwards.z));*/
	m_cam->setCameraDirection(forwards);
}

std::shared_ptr<Entity> PlayerController::getEntity() {
	return m_player;
}

void PlayerController::setProjectileModel(Model* model) {
	m_projectileModel = model;
}