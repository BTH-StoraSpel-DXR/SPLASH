#pragma once
#include "../BaseComponentSystem.h"
#include "Sail/graphics/camera/Camera.h"
#include "Sail/graphics/camera/PerspectiveCamera.h"
#include "Sail/graphics/light/PointLight.h"
#include "Sail/graphics/light/LightSetup.h"

class CameraRecordSystem final : public BaseComponentSystem {
public:
	CameraRecordSystem();
	~CameraRecordSystem() = default;

	void setCamera(Camera* camera);
	void setLightSetup(LightSetup* light, int index);
	void prepareRrender();
	void submitCamera(Camera* camera);
	void update(float dt);

	void renderImGUI();
	void startReplay();
	void stopReplay();
	bool isInReplay();

	void stop();
	Camera& getCamera();

private:
	struct Checkpoint {
		glm::vec3 pos;
		glm::vec3 direction;
		float timeStamp;
		float delta;
	};

	int m_currentCheckpoint = 0;
	float m_curTime = 0;
	float m_curTime2 = 0;
	bool m_replay = false;
	std::vector<Checkpoint> m_checkpoints;

	Camera* m_originalCam = nullptr;
	PerspectiveCamera m_replaycam;
	LightSetup* m_lightSetup;
	PointLight m_light;
};