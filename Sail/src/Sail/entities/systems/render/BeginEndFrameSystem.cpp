#include "pch.h"
#include "BeginEndFrameSystem.h"
#include "..//..//components/NoEntityComponent.h"
#include "..//..//..//Application.h"

BeginEndFrameSystem::BeginEndFrameSystem() {
	registerComponent<NoEntityComponent>(true, false, false);
}

BeginEndFrameSystem::~BeginEndFrameSystem() {
}

void BeginEndFrameSystem::renderNothing() {
	Renderer* renderer = Application::getInstance()->getRenderWrapper()->getCurrentRenderer();
	renderer->begin(nullptr);
	renderer->end();
	renderer->present();
}

void BeginEndFrameSystem::beginFrame(Camera& camera) {
	Application::getInstance()->getRenderWrapper()->getCurrentRenderer()->begin(&camera);
	Application::getInstance()->getRenderWrapper()->getScreenSpaceRenderer()->begin(&camera);
}

void BeginEndFrameSystem::endFrameAndPresent() {
	RendererWrapper* renderWrapper = Application::getInstance()->getRenderWrapper();
	renderWrapper->getCurrentRenderer()->end();
	renderWrapper->getCurrentRenderer()->present((renderWrapper->getDoPostProcessing()
		) ? renderWrapper->getPostProcessPipeline() : nullptr);
	
	renderWrapper->getScreenSpaceRenderer()->end();
	renderWrapper->getScreenSpaceRenderer()->present();
}