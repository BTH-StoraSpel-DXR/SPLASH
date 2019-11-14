#include "pch.h"
#include "DX12HybridRaytracerRenderer.h"
#include "DX12GBufferRenderer.h"
#include "DX12RaytracingRenderer.h"

DX12HybridRaytracerRenderer::DX12HybridRaytracerRenderer() {
	m_rendererGbuffer = std::make_unique<DX12GBufferRenderer>();
	m_rendererRaytrace = std::make_unique<DX12RaytracingRenderer>(m_rendererGbuffer->getGBufferOutputs());
}

DX12HybridRaytracerRenderer::~DX12HybridRaytracerRenderer() { 

}

void DX12HybridRaytracerRenderer::begin(Camera* camera) {
	m_rendererGbuffer->begin(camera);
	m_rendererRaytrace->begin(camera);
}

void DX12HybridRaytracerRenderer::submit(Mesh* mesh, const glm::mat4& modelMatrix, RenderFlag flags, int teamColorID) {
	teamColorID = std::clamp(teamColorID, 0, 12);

	if (flags & RenderFlag::IS_VISIBLE_ON_SCREEN) {
		m_rendererGbuffer->submit(mesh, modelMatrix, flags, teamColorID);
	}
	if (!(flags & RenderFlag::HIDE_IN_DXR)) {
		m_rendererRaytrace->submit(mesh, modelMatrix, flags, teamColorID);
	}
}

void DX12HybridRaytracerRenderer::submitMetaball(RenderCommandType type, Material* material, const glm::vec3& pos, RenderFlag flags) {
	if (flags & RenderFlag::IS_VISIBLE_ON_SCREEN) {
		m_rendererGbuffer->submitMetaball(type, material, pos, flags);
	}
	m_rendererRaytrace->submitMetaball(type, material, pos, flags);
}

void DX12HybridRaytracerRenderer::submitDecal(const glm::vec3& pos, const glm::mat3& rot, const glm::vec3& halfSize) {
	m_rendererRaytrace->submitDecal(pos, rot, halfSize);
}

void DX12HybridRaytracerRenderer::submitWaterPoint(const glm::vec3& pos) {
	m_rendererRaytrace->submitWaterPoint(pos);
}

void DX12HybridRaytracerRenderer::setLightSetup(LightSetup* lightSetup) {
	m_rendererGbuffer->setLightSetup(lightSetup);
	m_rendererRaytrace->setLightSetup(lightSetup);
}

void DX12HybridRaytracerRenderer::end() {
	m_rendererGbuffer->end();
	m_rendererRaytrace->end();
}

void DX12HybridRaytracerRenderer::present(PostProcessPipeline* postProcessPipeline, RenderableTexture* output) {
	m_rendererGbuffer->present();
	m_rendererRaytrace->present(postProcessPipeline, output);
}

bool DX12HybridRaytracerRenderer::onEvent(const Event& event) {
	m_rendererGbuffer->onEvent(event);
	m_rendererRaytrace->onEvent(event);
	return true;
}

void DX12HybridRaytracerRenderer::setTeamColors(const std::vector<glm::vec3>& teamColors) {
	m_rendererGbuffer->setTeamColors(teamColors);
	m_rendererRaytrace->setTeamColors(teamColors);
}

bool DX12HybridRaytracerRenderer::checkIfOnWater(const glm::vec3& pos) {
	return m_rendererRaytrace->checkIfOnWater(pos);
}
