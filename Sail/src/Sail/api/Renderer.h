#pragma once

#include <glm/glm.hpp>
#include "Sail/events/EventReceiver.h"

class Mesh;
class Camera;
class Model;
class LightSetup;
class RenderableTexture;
class PostProcessPipeline;
class Material;

class Renderer : public EventReceiver {
public:
	enum Type {
		FORWARD,
		DEFERRED,
		RAYTRACED,
		GBUFFER,
		HYBRID,
		SCREEN_SPACE,
		PARTICLES
	};

	enum RenderFlag {
		MESH_DYNAMIC			= 1 << 0,	// Vertices may change
		MESH_STATIC				= 1 << 1,	// Vertices will never change
		MESH_TRANSPARENT		= 1 << 2,	// Should be rendered see-through
		MESH_HERO				= 1 << 3,	// Mesh takes up a relatively large area of the screen 
		IS_VISIBLE_ON_SCREEN	= 1 << 4,	// Mesh should be rendered in raster pass(es)
		HIDE_IN_DXR				= 1 << 5	// Mesh should not be rendered in dxr pass(es)
	};

	enum RenderCommandType {
		RENDER_COMMAND_TYPE_MODEL,
		RENDER_COMMAND_TYPE_NON_MODEL_METABALL,
		RENDER_COMMAND_TYPE_NON_MODEL_DECAL
	};

	struct RenderCommand {
		RenderCommandType type;
		glm::mat4 transform;
		glm::mat4 transformLastFrame;
		RenderFlag flags = MESH_STATIC;
		std::vector<bool> hasUpdatedSinceLastRender;
		int teamColorID;
		bool castShadows;

		union {
			struct {
				Mesh* mesh;
			} model;
			struct {
				Material* material;
				int gpuGroupIndex;
			} metaball;
		};
	};

public:
	static Renderer* Create(Renderer::Type type);
	virtual ~Renderer() {}

	virtual void begin(Camera* camera);
	virtual void submit(Model* model, const glm::mat4& modelMatrix, RenderFlag flags, int teamColorID);
	virtual void submit(Model* model, const glm::mat4& modelMatrix, const glm::mat4& modelMatrixLastFrame, RenderFlag flags, int teamColorID);

	virtual void submitMetaball(RenderCommandType type, Material* material, const glm::vec3& pos, RenderFlag flags, int group) {}

	virtual void submitWaterPoint(const glm::vec3& pos) { };
	virtual bool checkIfOnWater(const glm::vec3& pos) { return false; }
	virtual unsigned int removeWaterPoint(const glm::vec3& pos, const glm::ivec3& posOffset, const glm::ivec3& negOffset) { return 0; };
	virtual std::pair<bool, glm::vec3> getNearestWaterPosition(const glm::vec3& position, const glm::vec3& maxOffset) { return std::pair(false, glm::vec3(0.f)); };
	virtual void end() { };

	virtual void submit(Mesh* mesh, const glm::mat4& modelMatrix, RenderFlag flags, int teamColorID, bool castShadows);
	virtual void submit(Mesh* mesh, const glm::mat4& modelMatrix, const glm::mat4& modelMatrixLastFrame, RenderFlag flags, int teamColorID, bool castShadows);
	virtual void setLightSetup(LightSetup* lightSetup);
	virtual void present(PostProcessPipeline* postProcessPipeline = nullptr, RenderableTexture* output = nullptr) = 0;
	virtual bool onEvent(const Event& event) override { return true; }

	virtual void setTeamColors(const std::vector<glm::vec3>& teamColors);

protected:
	std::vector<RenderCommand> commandQueue;
	std::vector<glm::vec3> teamColors;

	Camera* camera;
	LightSetup* lightSetup;
	
};

// Operators to use RenderFlags enum as bit flags
inline enum Renderer::RenderFlag operator|(const enum Renderer::RenderFlag a, const enum Renderer::RenderFlag b) {
	return static_cast<enum Renderer::RenderFlag>(static_cast<int>(a) | static_cast<int>(b));
}
inline enum Renderer::RenderFlag operator|=(enum Renderer::RenderFlag& a, const enum Renderer::RenderFlag& b) {
	a = static_cast<enum Renderer::RenderFlag>(static_cast<int>(a) | static_cast<int>(b));
	return a;
}