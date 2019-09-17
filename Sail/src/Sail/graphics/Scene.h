#pragma once

#include "../entities/Entity.h"
#include "camera/Camera.h"
#include "../events/Events.h"
//#include "postprocessing/PostProcessPipeline.h"


#include "Sail/graphics/geometry/transform/RenderTransform.h"

//TODO: Move elsewhere
class LightSetup;
class Renderer;
class Model;
class RenderTransform;

struct PerFrameRenderObject {
	Model* m_model = nullptr;
	RenderTransform* m_transform = nullptr;

	PerFrameRenderObject(Model* model, RenderTransform* transform)
		: m_model(model), m_transform(transform) {}

	virtual ~PerFrameRenderObject() {
		if (m_transform) {
			delete m_transform;
			m_transform = nullptr;
//			m_transform = nullptr;
		}
	}
};


// TODO: make this class virtual and have the actual scene in the demo/game project
class Scene : public IEventListener {
public:
	Scene();
	~Scene();

	// Adds an entity to later be drawn
	// This takes shared ownership of the entity
	void addEntity(Entity::SPtr entity);
	void setLightSetup(LightSetup* lights);
	void draw(Camera& camera, const float alpha = 1.0f);


	void prepareUpdate();

	void prepareRenderObjects();

	virtual bool onEvent(Event& event) override;

private:
	bool onResize(WindowResizeEvent& event);

private:
	// Entities are split into game objects and per-frame render objects to prevent
	// data races when rendering objects that have just been modified/deleted from
	// the scene.

	// Game objects are used for everything but the rendering pass
	//
	// Should include Model, Transform, Physics, Sound, etc.
	std::vector<Entity::SPtr> m_GameObjectEntities;




	// Render objects are essentially read only and have only the minimum required data
	// needed to render the corresponding game object. A list of render objects is created
	// at the end of each CPU update from the m_GameObjectEntities.
	//
	// should be a ring buffer or something similar
	// should only include Model, transform, and whatever else might be needed to
	// render the object.
	//std::vector<Entity::SPtr> m_perFrameRenderObjects[4];
	std::vector<PerFrameRenderObject> m_perFrameRenderObjects[SNAPSHOT_BUFFER_SIZE];
	std::mutex m_perFrameLocks[SNAPSHOT_BUFFER_SIZE];



	std::unique_ptr<Renderer> m_renderer;
	//DeferredRenderer m_renderer;
	//std::unique_ptr<DX11RenderableTexture> m_deferredOutputTex;
	//PostProcessPipeline m_postProcessPipeline;

};