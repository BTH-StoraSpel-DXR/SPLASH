#pragma once

#include "Sail/api/Renderer.h"
#include <glm/glm.hpp>
#include "../DX12API.h"
#include "../DXR/DXRBase.h"
#include "../resources/DX12RenderableTexture.h"

class DX12RaytracingRenderer : public Renderer {
public:
	DX12RaytracingRenderer();
	~DX12RaytracingRenderer();

	void present(PostProcessPipeline* postProcessPipeline = nullptr, RenderableTexture* output = nullptr) override;

	virtual bool onEvent(Event& event) override;

private:
	bool onResize(WindowResizeEvent& event);

private:
	DX12API* m_context;
	DX12API::Command m_command;
	DXRBase m_dxr;
	std::unique_ptr<DX12RenderableTexture> m_outputTexture;

};