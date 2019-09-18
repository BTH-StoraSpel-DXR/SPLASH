#pragma once

#include "Sail/api/Renderer.h"
//#include <glm/glm.hpp>

class DX11ForwardRenderer : public Renderer {
public:
	DX11ForwardRenderer();
	~DX11ForwardRenderer();

	void present(RenderableTexture* output = nullptr) override;

private:
	

};