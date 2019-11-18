#pragma once

#include "Sail/api/Renderer.h"
#include "../DX12API.h"

class DX12RenderableTexture;

class DX12ParticleRenderer : public Renderer {
public:
	DX12ParticleRenderer();
	~DX12ParticleRenderer();

	virtual void present(PostProcessPipeline* postProcessPipeline = nullptr, RenderableTexture* output = nullptr) override;
	virtual bool onEvent(const Event& event) override;

	void setDepthTexture(DX12RenderableTexture* tex);

private:
	DX12API* m_context;
	DX12API::Command m_command;
	DX12RenderableTexture* m_depthTexture;
};