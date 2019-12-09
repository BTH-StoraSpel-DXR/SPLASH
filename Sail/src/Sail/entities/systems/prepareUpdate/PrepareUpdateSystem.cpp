#include "pch.h"
#include "PrepareUpdateSystem.h"

#include "Sail/entities/Entity.h"
#include "Sail/entities/components/TransformComponent.h"
#include "Sail/entities/components/RenderInActiveGameComponent.h"

PrepareUpdateSystem::PrepareUpdateSystem() {
	// TODO: System owner should check if this is correct
	registerComponent<TransformComponent>(true, true, true);
	registerComponent<RenderInActiveGameComponent>(true, false, false);
}

PrepareUpdateSystem::~PrepareUpdateSystem() {

}

void PrepareUpdateSystem::fixedUpdate() {
	for (auto e : entities) {
		e->getComponent<TransformComponent>()->prepareFixedUpdate();
	}
}

void PrepareUpdateSystem::update() {
	for (auto e : entities) {
		e->getComponent<TransformComponent>()->prepareUpdate();
	}
}

