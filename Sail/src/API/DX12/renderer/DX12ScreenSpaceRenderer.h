#pragma once

#include "Sail/api/Renderer.h"
#include "../DX12API.h"

class DX12ScreenSpaceRenderer : public Renderer {
public:
	DX12ScreenSpaceRenderer();
	~DX12ScreenSpaceRenderer();

	virtual void present(PostProcessPipeline* postProcessPipeline = nullptr, RenderableTexture* output = nullptr) override;
	virtual bool onEvent(Event& event) override;

private:
	DX12API* m_context;
	DX12API::Command m_command;
};