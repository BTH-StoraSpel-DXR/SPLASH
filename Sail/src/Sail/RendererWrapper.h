#pragma once

#include "Sail/events/IEventListener.h"

class Renderer;
class LightSetup;
class PostProcessPipeline;

class RendererWrapper : public IEventListener {
public:
	RendererWrapper();
	virtual ~RendererWrapper();

	void initialize();

	void changeRenderer(unsigned int index);
	void togglePostProcessing(const bool flag);
	bool& getDoPostProcessing();
	Renderer* getCurrentRenderer();
	Renderer* getScreenSpaceRenderer();
	PostProcessPipeline* getPostProcessPipeline();
	bool onEvent(Event& event);
	bool onResize(Event& event);

private:
	void setLightSetup(LightSetup* lights);

	std::unique_ptr<Renderer> m_rendererRaster;
	std::unique_ptr<Renderer> m_rendererRaytrace;
	std::unique_ptr<Renderer> m_rendererScreenSpace;
	Renderer* m_currentRenderer;
	std::shared_ptr<PostProcessPipeline> m_postProcessPipeline;

	bool m_doPostProcessing;
};