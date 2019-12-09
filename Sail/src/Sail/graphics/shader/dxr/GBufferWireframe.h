#pragma once

#include <glm/glm.hpp>
#include "Sail/api/shader/ShaderPipeline.h"
#include "Sail/graphics/shader/Shader.h"

class GBufferWireframe : public Shader {
public:
	GBufferWireframe();
	~GBufferWireframe();

	virtual void bind() override;
	virtual void setClippingPlane(const glm::vec4& clippingPlane) override;

private:


private:
	glm::vec4 m_clippingPlane;
	bool m_clippingPlaneHasChanged;
};
