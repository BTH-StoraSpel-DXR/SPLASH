#pragma once

#include "Sail/events/EventReceiver.h"

class Renderer;
class LightSetup;
class PostProcessPipeline;

class RendererWrapper : public EventReceiver {
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
	bool checkIfOnWater(const glm::vec3& worldPos) const;
	bool onEvent(const Event& event) override;

private:
	void setLightSetup(LightSetup* lights);

	std::unique_ptr<Renderer> m_rendererRaster;
	std::unique_ptr<Renderer> m_rendererRaytrace;
	std::unique_ptr<Renderer> m_rendererScreenSpace;
	Renderer* m_currentRenderer;
	std::shared_ptr<PostProcessPipeline> m_postProcessPipeline;

	bool m_doPostProcessing;
};