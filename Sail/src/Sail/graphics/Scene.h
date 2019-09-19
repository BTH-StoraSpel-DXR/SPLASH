#pragma once

#include "../entities/Entity.h"
#include "camera/Camera.h"
#include "../events/Events.h"
#include "postprocessing/PostProcessPipeline.h"

class LightSetup;
class Renderer;
// TODO: make this class virtual and have the actual scene in the demo/game project
class Scene : public IEventListener {
public:
	Scene();
	~Scene();

	// Adds an entity to later be drawn
	// This takes shared ownership of the entity
	void addEntity(Entity::SPtr entity);
	void setLightSetup(LightSetup* lights);
	void showBoundingBoxes(bool val);
	Entity::SPtr getEntityByName(std::string name);
	const std::vector<Entity::SPtr>& getEntities()const;
	void draw(void);
	void draw(Camera& camera, const float alpha = 1.0f);


	void prepareUpdate();

	virtual bool onEvent(Event& event) override;
	void changeRenderer(unsigned int index);
	bool& getDoProcessing();

private:
	bool onResize(WindowResizeEvent& event);

private:
	std::vector<Entity::SPtr> m_entities;
	std::unique_ptr<Renderer> m_rendererRaster;
	std::unique_ptr<Renderer> m_rendererRaytrace;
	std::unique_ptr<Renderer>* m_currentRenderer;
	PostProcessPipeline m_postProcessPipeline;
	bool m_doPostProcessing;
	bool m_showBoundingBoxes;
};