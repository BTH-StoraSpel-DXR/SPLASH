#include "pch.h"
#include "GunComponent.h"

GunComponent::GunComponent() {
	 projectileSpawnTimer = 0.f;
	 gunOverloadTimer = 0.f;
	 m_projectileSpawnCooldown = 0.015f;
	 m_gunOverloadCooldown = .00f;

	 // OLD
	 projectileSpeed = 10.f;
	 // NEW
	 baseProjectileSpeed = projectileSpeed;
	 finalProjectileSpeed = baseProjectileSpeed * 0.5f;
	 projectileSpeedRange = finalProjectileSpeed - baseProjectileSpeed;


	 gunOverloadvalue = 0.f;
	 gunOverloadThreshold = 1.0f;

	firing = false;
}

void GunComponent::setFiring(glm::vec3 pos, glm::vec3 dir) {
	firing = true;
	position = pos;
	direction = dir;
}