#pragma once

#include "Sail/graphics/shader/Shader.h"

class ShadePassShader : public Shader {
public:
	ShadePassShader();
	~ShadePassShader();

	virtual void bind() override;

};
