#pragma once

#include "Sail/api/Renderer.h"

class DX12GBufferRenderer;
class DX12RaytracingRenderer;

class DX12HybridRaytracerRenderer : public Renderer {
public:
	DX12HybridRaytracerRenderer();
	~DX12HybridRaytracerRenderer();

	virtual void begin(Camera* camera) override;
	virtual void submit(Mesh* mesh, const glm::mat4& modelMatrix, RenderFlag flags, int teamColorID, bool castShadows) override;
	virtual void submitMetaball(RenderCommandType type, Material* material, const glm::vec3& pos, RenderFlag flags, const int group);

	virtual void submitDecal(const glm::vec3& pos, const glm::mat3& rot, const glm::vec3& halfSize) override;
	virtual void submitWaterPoint(const glm::vec3& pos) override;
	virtual void setLightSetup(LightSetup* lightSetup) override;
	virtual void end() override;
	virtual void present(PostProcessPipeline* postProcessPipeline = nullptr, RenderableTexture* output = nullptr) override;
	virtual bool onEvent(const Event& event) override;
	virtual void setTeamColors(const std::vector<glm::vec3>& teamColors) override;
	virtual bool checkIfOnWater(const glm::vec3& pos) override;

	DX12GBufferRenderer* getGBufferRenderer() const;

private:
	std::unique_ptr<DX12GBufferRenderer> m_rendererGbuffer;
	std::unique_ptr<DX12RaytracingRenderer> m_rendererRaytrace;

};