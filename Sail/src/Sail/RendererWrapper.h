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
	Renderer* getParticleRenderer();
	Renderer* getScreenSpaceRenderer();
	PostProcessPipeline* getPostProcessPipeline();
	unsigned int removeWaterPoint(const glm::vec3& worldPos, const glm::ivec3& posOffset, const glm::ivec3& negOffset);
	bool checkIfOnWater(const glm::vec3& worldPos) const;
	virtual std::pair<bool, glm::vec3> getNearestWaterPosition(const glm::vec3& position, const glm::vec3& maxOffset);
	bool onEvent(const Event& event) override;

private:
	void setLightSetup(LightSetup* lights);

	std::unique_ptr<Renderer> m_rendererRaster;
	std::unique_ptr<Renderer> m_rendererRaytrace;
	std::unique_ptr<Renderer> m_rendererScreenSpace;
	std::unique_ptr<Renderer> m_rendererParticles;
	std::unique_ptr<PostProcessPipeline> m_postProcessPipeline;
	Renderer* m_currentRenderer;

	bool m_doPostProcessing;
};