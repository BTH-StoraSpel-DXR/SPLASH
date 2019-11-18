#pragma once
#include "../BaseComponentSystem.h"
#include "Sail/entities/Entity.h"
class Camera;
class CameraController;
class GameDataTracker;
class LevelSystem;

#define DETECTION_STEP_SIZE 0.35f

struct Movement {
	float speedModifier = 1.f;
	float forwardMovement = 0.0f;
	float rightMovement = 0.0f;
	float upMovement = 0.0f;
};

class GameInputSystem final : public BaseComponentSystem {
public:
	GameInputSystem();
	~GameInputSystem();

	void fixedUpdate(float dt);
	void update(float dt, float alpha);
	void initialize(Camera* cam);
	void clean();
	void stop() override;
	void updateCameraPosition(float alpha);
	CameraController* getCamera() const;

	LevelSystem* m_mapPointer;

private:
	void processKeyboardInput(const float& dt);
	void processMouseInput(const float& dt);

	// For both carrying and throwing
	void toggleCandleCarry(Entity* entity);
	Movement getPlayerMovementInput(Entity* entity);
	
	CameraController* m_cam = nullptr;
	GameDataTracker* m_gameDataTracker = nullptr;


	float m_candleToggleTimer = 0.0f;

	// --------- Earlier used variables below this line ---------
	float m_runSpeed = 2.0;
	float m_movementSpeed = 20.f;
	bool m_wasSpacePressed = false;
	float m_projectileSpawnCounter = 0.f;
	float m_lookSensitivityMouse = 0.1f;
	glm::vec3 m_playerPosHolder[5];
	bool m_isOnWaterHolder = false;

	// Sound-related Variables
	float m_onGroundTimer = 0.0f;
	float m_onGroundThreshold = 0.3f;
	float m_isPlayingRunningSound = false;
	float m_fallTimer = 0.0f;
	float m_fallThreshold = 0.6f;
	float m_soundSwitchTimer = 0.0f;
	float m_changeThreshold = 0.358f; /*(0.756f / 2), 0.756 = all footstep sounds containing 2 steps each*/
	bool m_killSoundUponDeath = true;

	// TEMP BOOLS
	bool tempMetal = false;
	bool tempTile = false;
	bool tempWaterMetal = false;
	bool tempWaterTile = false;
	bool tempStopAll = false;

	// #netcodeNote not thread safe, might cause issues
	float m_yaw, m_pitch, m_roll;
};