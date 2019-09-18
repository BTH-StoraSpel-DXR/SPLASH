#pragma once

#include "Sail/api/Renderer.h"
//#include <glm/glm.hpp>
#include "../DX12API.h"

class DX12ForwardRenderer : public Renderer {
public:
	DX12ForwardRenderer();
	~DX12ForwardRenderer();

	void present(RenderableTexture* output = nullptr) override;

private:
	DX12API* m_context;
	DX12API::Command m_command;

};