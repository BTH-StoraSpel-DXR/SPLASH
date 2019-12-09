#include "pch.h"
#include "GBufferWireframe.h"
#include "Sail/Application.h"
#include "API/DX12/shader/DX12ShaderPipeline.h"

GBufferWireframe::GBufferWireframe()
	: Shader("dxr/GBufferWireframe.hlsl")
	, m_clippingPlaneHasChanged(false)
{
	// Create the input layout
	shaderPipeline->getInputLayout().pushVec3(InputLayout::POSITION, "POSITION", 0);
	shaderPipeline->getInputLayout().pushVec3(InputLayout::NORMAL, "NORMAL", 0);
	shaderPipeline->getInputLayout().create(shaderPipeline->getVsBlob());
	setWireframe(true);

	shaderPipeline->setNumRenderTargets(4);

	// Finish the shader creation
	finish();
}
GBufferWireframe::~GBufferWireframe() {

}

void GBufferWireframe::setClippingPlane(const glm::vec4& clippingPlane) {
	m_clippingPlane = clippingPlane;
	m_clippingPlaneHasChanged = true;
}

void GBufferWireframe::bind() {

	// Call parent to bind shaders
	Shader::bind();

}
