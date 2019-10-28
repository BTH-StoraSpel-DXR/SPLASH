#include "pch.h"
#include "RendererWrapper.h"

#include "api/Renderer.h"
#include "graphics/postprocessing/PostProcessPipeline.h"
#include "Sail/events/WindowResizeEvent.h"

RendererWrapper::RendererWrapper() {
}

RendererWrapper::~RendererWrapper() {
}

void RendererWrapper::initialize() {
	m_rendererRaster = std::unique_ptr<Renderer>(Renderer::Create(Renderer::FORWARD));
	m_rendererRaytrace = std::unique_ptr<Renderer>(Renderer::Create(Renderer::HYBRID));
	m_rendererScreenSpace = std::unique_ptr<Renderer>(Renderer::Create(Renderer::SCREEN_SPACE));
	m_currentRenderer = m_rendererRaytrace.get();

	m_postProcessPipeline = std::make_shared<PostProcessPipeline>();

	m_doPostProcessing = false;
}

/*
	0 : Raytrace
	1 : Raster
*/
void RendererWrapper::changeRenderer(unsigned int index) {

	switch (index) {
	case 0:
		m_currentRenderer = m_rendererRaytrace.get();
		break;
	case 1:
		m_currentRenderer = m_rendererRaster.get();
		break;
	default:
		break;
	}
}

void RendererWrapper::togglePostProcessing(const bool flag) {
	m_doPostProcessing = flag;
}

bool& RendererWrapper::getDoPostProcessing() {
	return m_doPostProcessing;
}

Renderer* RendererWrapper::getCurrentRenderer() {
	return m_currentRenderer;
}

PostProcessPipeline* RendererWrapper::getPostProcessPipeline() {
	return m_postProcessPipeline.get();
}

bool RendererWrapper::onEvent(Event& event) {

	EventHandler::dispatch<WindowResizeEvent>(event, SAIL_BIND_EVENT(&RendererWrapper::onResize));

	return true;
}

bool RendererWrapper::onResize(Event& event) {

	m_rendererRaster->onEvent(event);
	m_rendererRaytrace->onEvent(event);

	return true;
}

void RendererWrapper::setLightSetup(LightSetup* lights) {
	m_currentRenderer->setLightSetup(lights);
}


Renderer* RendererWrapper::getScreenSpaceRenderer() {
	return m_rendererScreenSpace.get();
}