#include "pch.h"
#include "LightSystem.h"

#include "Sail/entities/components/LightComponent.h"
#include "Sail/entities/components/RealTimeComponent.h"
#include "Sail/entities/components/RenderInActiveGameComponent.h"
#include "Sail/entities/components/RenderInReplayComponent.h"
#include "Sail/entities/ECS.h"
#include "Sail/graphics/light/LightSetup.h"

template <typename T>
LightSystem<T>::LightSystem() : BaseComponentSystem() {
	// TODO: System owner should check if this is correct
	registerComponent<LightComponent>(true, true, true);
	registerComponent<T>(true, false, false);
}

template <typename T>
LightSystem<T>::~LightSystem() {
}

template<typename T>
void LightSystem<T>::prepareFixedUpdate() {
	for (auto e : entities) {
		e->getComponent<LightComponent>()->prevPos = e->getComponent<LightComponent>()->currentPos;
	}
}



//check and update all lights for all entities
template <typename T>
void LightSystem<T>::updateLights(LightSetup* lightSetup, const float alpha) {
	for (auto e : entities) {
		LightComponent* lc = e->getComponent<LightComponent>();

		// Interpolate light positions if it's not a real-time entity
		if (!e->hasComponent<RealTimeComponent>()) {
			lc->getPointLight().setPosition(lc->getInterpolatedPosition(alpha));
		} else {
			lc->getPointLight().setPosition(lc->currentPos);
		}

		lightSetup->addPointLight(lc->getPointLight());
	}
}



template class LightSystem<RenderInActiveGameComponent>;
template class LightSystem<RenderInReplayComponent>;
