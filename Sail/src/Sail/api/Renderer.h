#pragma once

#include <glm/glm.hpp>
#include "Sail/events/Events.h"

class Mesh;
class Camera;
class Model;
class LightSetup;
class RenderableTexture;
class PostProcessPipeline;

class Renderer : public IEventListener {
public:
	enum Type {
		FORWARD,
		DEFERRED,
		RAYTRACED
	};
	struct RenderCommand {
		Mesh* mesh;
		glm::mat4 transform; // TODO: find out why having a const ptr here doesnt work
	};
public:
	static Renderer* Create(Renderer::Type type);
	virtual ~Renderer() {}

	virtual void begin(Camera* camera);
	void submit(Model* model, const glm::mat4& modelMatrix);
	virtual void submit(Mesh* mesh, const glm::mat4& modelMatrix);
	virtual void setLightSetup(LightSetup* lightSetup);
	virtual void end();
	virtual void present(PostProcessPipeline* postProcessPipeline = nullptr, RenderableTexture* output = nullptr) = 0;
	virtual bool onEvent(Event& event) override { return true; };

protected:
	std::vector<RenderCommand> commandQueue;
	Camera* camera;
	LightSetup* lightSetup;

};