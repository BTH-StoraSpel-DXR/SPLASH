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
	RendererWrapper* renderWrapper = Application::getInstance()->getRenderWrapper();
	Renderer* renderer = renderWrapper->getCurrentRenderer();
	renderer->setLightSetup(nullptr);
	renderer->begin(nullptr);
	renderer->end();
	renderer->present((renderWrapper->getDoPostProcessing()) ? renderWrapper->getPostProcessPipeline() : nullptr);
}

void BeginEndFrameSystem::beginFrame(Camera& camera) {
	Application::getInstance()->getRenderWrapper()->getCurrentRenderer()->begin(&camera);
	//Application::getInstance()->getRenderWrapper()->getScreenSpaceRenderer()->begin(&camera);
	Application::getInstance()->getRenderWrapper()->getParticleRenderer()->begin(&camera);
}

void BeginEndFrameSystem::endFrameAndPresent() {
	RendererWrapper* renderWrapper = Application::getInstance()->getRenderWrapper();
	renderWrapper->getCurrentRenderer()->end();
	renderWrapper->getCurrentRenderer()->present((renderWrapper->getDoPostProcessing()
		) ? renderWrapper->getPostProcessPipeline() : nullptr);
	
	renderWrapper->getParticleRenderer()->end();
	renderWrapper->getParticleRenderer()->present();

	//renderWrapper->getScreenSpaceRenderer()->end();
	//renderWrapper->getScreenSpaceRenderer()->present();

}