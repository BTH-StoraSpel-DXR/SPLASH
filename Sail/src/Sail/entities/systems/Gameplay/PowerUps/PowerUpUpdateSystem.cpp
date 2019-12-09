
#include "pch.h" 
#include "PowerUpUpdateSystem.h"
#include "Sail/entities/components/PowerUp/PowerUpComponent.h"

PowerUpUpdateSystem::PowerUpUpdateSystem() {
	registerComponent<PowerUpComponent>(true, true, true);
}

PowerUpUpdateSystem::~PowerUpUpdateSystem() {
}

void PowerUpUpdateSystem::update(float dt) {
	for (auto& e: entities) {
		if (auto* powC = e->getComponent<PowerUpComponent>()) {
			for (auto& pow : powC->powerUps) {
				pow.time -= dt;
				if (pow.time < 0) {
					pow.time = 0;
				}
			}
		}
	}
}
#ifdef DEVELOPMENT
unsigned int PowerUpUpdateSystem::getByteSize() const {
	int size = sizeof(*this);
	return size;
}
#endif