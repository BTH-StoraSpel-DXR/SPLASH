#pragma once
#include "Component.h"
#include <glm/glm.hpp>

class Model;

class GunComponent : public Component<GunComponent> {
public:
	GunComponent(Model* projectileModel, Model* wireframeModel) : 
	m_projectileModel(projectileModel)
	,m_wireframeModel(wireframeModel)
	{ };
	~GunComponent() { };

	Model* getProjectileModel() const {
		return m_projectileModel;
	}

	Model* getWireframeModel() const {
		return m_wireframeModel;
	}

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
	bool firingContinuously = false;

private:
	Model* m_projectileModel;
	Model* m_wireframeModel;
	float m_projectileSpawnLimit = .3f;
};
