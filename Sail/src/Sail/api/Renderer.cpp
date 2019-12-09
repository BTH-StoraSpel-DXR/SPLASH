#include "pch.h"
#include "Renderer.h"
#include "Sail/graphics/geometry/Model.h"

void Renderer::begin(Camera* camera) {
	this->camera = camera;
	commandQueue.clear();
}

void Renderer::submit(Model* model, const glm::mat4& modelMatrix, RenderFlag flags, int teamColorID) {
	for (unsigned int i = 0; i < model->getNumberOfMeshes(); i++) {
		submit(model->getMesh(i), modelMatrix, flags, teamColorID, model->castsShadows());
	}
}

void Renderer::submit(Model* model, const glm::mat4& modelMatrix, const glm::mat4& modelMatrixLastFrame, RenderFlag flags, int teamColorID) {
	for (unsigned int i = 0; i < model->getNumberOfMeshes(); i++) {
		submit(model->getMesh(i), modelMatrix, modelMatrixLastFrame, flags, teamColorID, model->castsShadows());
	}
}

void Renderer::submit(Mesh* mesh, const glm::mat4& modelMatrix, RenderFlag flags, int teamColorID, bool castShadows) {
	RenderCommand cmd;
	cmd.type = RENDER_COMMAND_TYPE_MODEL;
	cmd.model.mesh = mesh;
	cmd.transform = glm::transpose(modelMatrix);
	cmd.flags = flags;
	cmd.teamColorID = teamColorID;
	cmd.castShadows = castShadows;
	commandQueue.push_back(cmd);
}

void Renderer::submit(Mesh* mesh, const glm::mat4& modelMatrix, const glm::mat4& modelMatrixLastFrame, RenderFlag flags, int teamColorID, bool castShadows) {
	RenderCommand cmd;
	cmd.type = RENDER_COMMAND_TYPE_MODEL;
	cmd.model.mesh = mesh;
	cmd.transform = glm::transpose(modelMatrix);
	cmd.transformLastFrame = glm::transpose(modelMatrixLastFrame);
	cmd.flags = flags;
	cmd.teamColorID = teamColorID;
	cmd.castShadows = castShadows;
	commandQueue.push_back(cmd);
}

void Renderer::setLightSetup(LightSetup* lightSetup) {
	this->lightSetup = lightSetup;
}

void Renderer::setTeamColors(const std::vector<glm::vec3>& teamColors) {
	this->teamColors = teamColors;
}

