#pragma once
#include "Component.h"
#include <glm/glm.hpp>

class Model;

enum GunState {
	STARTING,
	LOOPING,
	ENDING,
	STANDBY
};

class GunComponent : public Component<GunComponent> {
public:
	GunComponent() {};
	~GunComponent() {};

	void setFiring(glm::vec3 pos, glm::vec3 dir);

	glm::vec3 position;
	glm::vec3 direction;

	float projectileSpawnTimer = 0.f;
	float gunOverloadTimer = 0.f;
	float m_projectileSpawnCooldown = 0.02f;
	float m_gunOverloadCooldown = .5f;


	float projectileSpeed = 15.f;

	float gunOverloadvalue = 0.f;
	float gunOverloadThreshold = .5f;

	bool firing = false;
	
	// Used to manage sound in gunsystem.
	bool firingContinuously = false;
	GunState state = GunState::STANDBY;

private:
	float m_projectileSpawnLimit = .3f;
};
