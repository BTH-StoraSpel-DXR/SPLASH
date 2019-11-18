#pragma once

#include "Sail/graphics/shader/Shader.h"

class GBufferOutShaderNoDepth : public Shader {
public:
	GBufferOutShaderNoDepth();
	~GBufferOutShaderNoDepth();

	virtual void bind() override;

};
