#include "pch.h"
#include "ShadePassShader.h"
#include "API/DX12/shader/DX12ShaderPipeline.h"

ShadePassShader::ShadePassShader()
	: Shader("dxr/shading/ShadePass.hlsl")
{
	// Create the input layout
	shaderPipeline->getInputLayout().pushVec3(InputLayout::POSITION, "POSITION", 0);
	shaderPipeline->getInputLayout().create(shaderPipeline->getVsBlob());

	shaderPipeline->enableDepthStencil(false);
	shaderPipeline->setNumRenderTargets(2);
	static_cast<DX12ShaderPipeline*>(shaderPipeline)->setRenderTargetFormat(0, DXGI_FORMAT_R16G16B16A16_FLOAT);
	static_cast<DX12ShaderPipeline*>(shaderPipeline)->setRenderTargetFormat(1, DXGI_FORMAT_R16G16B16A16_FLOAT);

	// Finish the shader creation
	finish();
}
ShadePassShader::~ShadePassShader() {

}

void ShadePassShader::bind() {
	// Call parent to bind shaders
	Shader::bind();
}
