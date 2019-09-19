#pragma once

#include <glm/vec4.hpp>
#include "Sail/graphics/shader/Shader.h"

class MaterialShader : public Shader {
public:
	MaterialShader();
	~MaterialShader();

	virtual void bind() override;
	virtual void setClippingPlane(const glm::vec4& clippingPlane) override;

private:
	glm::vec4 m_clippingPlane;
	bool m_clippingPlaneHasChanged;
};
