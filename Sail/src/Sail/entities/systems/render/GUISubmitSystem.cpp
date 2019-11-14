#include "pch.h"
#include "GUISubmitSystem.h"
#include "Sail/Application.h"
#include "Sail/entities/components/Components.h"


GUISubmitSystem::GUISubmitSystem() {
	registerComponent<GUIComponent>(true, false, false);
}

GUISubmitSystem::~GUISubmitSystem() {
}

void GUISubmitSystem::submitAll() {
	Renderer* renderer = Application::getInstance()->getRenderWrapper()->getScreenSpaceRenderer();
	for (auto& e : entities) {
		Renderer::RenderFlag flags = Renderer::MESH_STATIC;
		Model* model = e->getComponent<GUIComponent>()->getCurrentModel();
		renderer->submit(model, glm::identity<glm::mat4>(), flags, 0);
	}
}