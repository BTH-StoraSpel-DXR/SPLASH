#pragma once

#ifdef _DEBUG
#define SAIL_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#endif

#include <d3d11.h>
#include <glm/glm.hpp>
#include <vector>
#include "../../events/Events.h"
#include "Sail/api/RenderableTexture.h"

class Camera;
class Model;
class Mesh;
class LightSetup;

class Renderer : public IEventListener {
public:
	struct RenderCommand {
		Mesh* mesh;
		glm::mat4 transform; // TODO: find out why having a const ptr here doesnt work
	};

public:
	Renderer();
	~Renderer();

	virtual void begin(Camera* camera) = 0;
	void submit(Model* model, const glm::mat4& modelMatrix);
	virtual void submit(Mesh* mesh, const glm::mat4& modelMatrix) = 0;
	virtual void setLightSetup(LightSetup* lightSetup) = 0;
	virtual void end() = 0;
	virtual void present(RenderableTexture* output = nullptr) = 0;
	virtual bool onEvent(Event& event) override { };
	
protected:
	std::vector<RenderCommand> commandQueue;

};
