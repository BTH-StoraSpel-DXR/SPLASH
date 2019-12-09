#include "pch.h"
#include "LightListSystem.h"
#include "../../components/LightListComponent.h"
#include "../../Entity.h"
#include "Sail/graphics/light/LightSetup.h"

#ifdef _DEBUG
#include "Sail/graphics/camera/PerspectiveCamera.h"
#endif

LightListSystem::LightListSystem() {
	registerComponent<LightListComponent>(true, true, true);
}

LightListSystem::~LightListSystem() {
}

void LightListSystem::updateLights(LightSetup* lightSetup) {
	for (auto& e : entities) {
		LightListComponent* lightList = e->getComponent<LightListComponent>();
		for (auto& light : lightList->getLightList()) {
			lightSetup->addPointLight(light);
		}
	}
}

#ifdef _DEBUG
void LightListSystem::setDebugLightListEntity(std::string name) {
	for (auto e : entities) {
		if (e->getName() == name) {
			e->addComponent<LightListComponent>();
			m_debugLightListEntity = e;
			break;
		}
	}
}

void LightListSystem::addPointLightToDebugEntity(LightSetup* lightSetup, PerspectiveCamera* cam) {
	PointLight pl;
	pl.setColor(glm::vec3(Utils::rnd(), Utils::rnd(), Utils::rnd()));
	pl.setPosition(cam->getPosition());

	if (m_debugLightListEntity) {
		LightListComponent* llc = m_debugLightListEntity->getComponent<LightListComponent>();
		if (llc) {
			llc->getLightList().push_back(pl);
		}
	}
	else {
		SAIL_LOG_WARNING("m_debugLightListEntity not setup, point light could not be added.");
	}
}

void LightListSystem::removePointLightFromDebugEntity() {
	// Remove the most recently added light from the lightList
	if (m_debugLightListEntity) {
		LightListComponent* llc = m_debugLightListEntity->getComponent<LightListComponent>();
		if (llc && llc->getLightList().size() > 0) {
			llc->getLightList().erase(llc->getLightList().begin());
		}
	}
}
#endif