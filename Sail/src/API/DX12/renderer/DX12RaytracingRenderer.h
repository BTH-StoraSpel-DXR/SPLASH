#pragma once

#include "Sail/api/Renderer.h"
#include <glm/glm.hpp>
#include "../DX12API.h"
#include "../DXR/DXRBase.h"
#include "../resources/DX12RenderableTexture.h"

class DX12RaytracingRenderer final : public Renderer {
public:
	DX12RaytracingRenderer(DX12RenderableTexture** inputs);
	~DX12RaytracingRenderer();

	void present(PostProcessPipeline* postProcessPipeline = nullptr, RenderableTexture* output = nullptr) override;
	virtual void begin(Camera* camera) override;

	virtual bool onEvent(const Event& event) override;
	virtual void submit(Mesh* mesh, const glm::mat4& modelMatrix, RenderFlag flags, int teamColorID) override;
	virtual void submitMetaball(RenderCommandType type, Material* material, const glm::vec3& pos, RenderFlag flags) override;
	virtual void submitDecal(const glm::vec3& pos, const glm::mat3& rot, const glm::vec3& halfSize) override;
	virtual void submitWaterPoint(const glm::vec3& pos) override;
	virtual bool checkIfOnWater(const glm::vec3& pos) override;

	virtual void setTeamColors(const std::vector<glm::vec3>& teamColors);
	virtual void updateMetaballAABB();

	void setGBufferInputs(DX12RenderableTexture** inputs);

private:
	bool onResize(const WindowResizeEvent& event);

private:
	DX12API* m_context;
	DXRBase m_dxr;
	DX12API::Command m_commandDirect;
	DX12API::Command m_commandCompute;
	std::unique_ptr<DX12RenderableTexture> m_outputTexture;
	std::unique_ptr<DX12RenderableTexture> m_outputBloomTexture;

	std::vector<DXRBase::Metaball> m_metaballs;
	D3D12_RAYTRACING_AABB m_nextMetaballAabb;

	// Decals
	DXRShaderCommon::DecalData m_decals[MAX_DECALS];
	size_t m_currNumDecals;
};
