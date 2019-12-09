#include "pch.h"
#include "GBufferOutShader.h"
#include "Sail/Application.h"
#include "API/DX12/shader/DX12ShaderPipeline.h"

GBufferOutShader::GBufferOutShader()
	: Shader("dxr/GBufferOut.hlsl")
	, m_clippingPlaneHasChanged(false) {
	// Create the input layout
	shaderPipeline->getInputLayout().pushVec3(InputLayout::POSITION, "POSITION", 0);
	shaderPipeline->getInputLayout().pushVec2(InputLayout::TEXCOORD, "TEXCOORD", 0);
	shaderPipeline->getInputLayout().pushVec3(InputLayout::NORMAL, "NORMAL", 0);
	shaderPipeline->getInputLayout().pushVec3(InputLayout::TANGENT, "TANGENT", 0);
	shaderPipeline->getInputLayout().pushVec3(InputLayout::BITANGENT, "BINORMAL", 0);
	shaderPipeline->getInputLayout().create(shaderPipeline->getVsBlob());

	// Enable back-face culling
	shaderPipeline->setCullMode(GraphicsAPI::BACKFACE);
	shaderPipeline->setNumRenderTargets(4);
	static_cast<DX12ShaderPipeline*>(shaderPipeline)->setRenderTargetFormat(3, DXGI_FORMAT_R16G16_FLOAT);

	// Finish the shader creation
	finish();
}
GBufferOutShader::~GBufferOutShader() { }

void GBufferOutShader::setClippingPlane(const glm::vec4& clippingPlane) {
	m_clippingPlane = clippingPlane;
	m_clippingPlaneHasChanged = true;
}

void GBufferOutShader::bind() {

	// Call parent to bind shaders
	Shader::bind();

}
