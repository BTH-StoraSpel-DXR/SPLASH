#include "pch.h"
#include "CrosshairHitSystem.h"
#include "../../ECS.h"
#include "Sail/entities/components/Components.h"


CrosshairHitSystem::CrosshairHitSystem() {
	registerComponent<CrosshairHitComponent>(true, true, true);
}

CrosshairHitSystem::~CrosshairHitSystem() {


}

void CrosshairHitSystem::update(float dt) {
	for (auto& e : entities) {
		CrosshairHitComponent* c = e->getComponent<CrosshairHitComponent>();

		if (c->recentlyHitSomeone) {
			
			alterCrosshair(e, dt);

		}
	}
}

void CrosshairHitSystem::alterCrosshair(Entity* e, float dt) {
	// Updates the crosshair based on how much time has passed
	// ---------------- 
	CrosshairHitComponent* c = e->getComponent<CrosshairHitComponent>();
	c->passedTimeSinceAlteration += dt;

	if (c->passedTimeSinceAlteration >= c->durationOfAlteredCrosshair) {
		c->recentlyHitSomeone = false;
		c->passedTimeSinceAlteration = 0;
		e->getComponent<GUIComponent>()->getCurrentModel()->getMesh(0)->getMaterial()->setAlbedoTexture(
			c->normalTexture
		);
	}
	else {
		// Do the actual change here, moving the crosshair or changing its color etc.
		e->getComponent<GUIComponent>()->getCurrentModel()->getMesh(0)->getMaterial()->setAlbedoTexture(
			c->alteredTexture
		);
	}
}
