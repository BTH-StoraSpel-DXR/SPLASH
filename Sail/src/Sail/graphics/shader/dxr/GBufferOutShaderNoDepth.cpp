#include "pch.h"
#include "GBufferOutShaderNoDepth.h"
#include "Sail/Application.h"
#include "API/DX12/shader/DX12ShaderPipeline.h"

GBufferOutShaderNoDepth::GBufferOutShaderNoDepth()
	: Shader("dxr/ParticleShader.hlsl")
{
	// Create the input layout
	shaderPipeline->getInputLayout().pushVec3(InputLayout::POSITION, "POSITION", 0);
	shaderPipeline->getInputLayout().pushVec2(InputLayout::TEXCOORD, "TEXCOORD", 0);
	shaderPipeline->getInputLayout().pushVec3(InputLayout::NORMAL, "NORMAL", 0);
	shaderPipeline->getInputLayout().pushVec3(InputLayout::TANGENT, "TANGENT", 0);
	shaderPipeline->getInputLayout().pushVec3(InputLayout::BITANGENT, "BINORMAL", 0);
	shaderPipeline->getInputLayout().create(shaderPipeline->getVsBlob());

	// Disable depth writing
	getPipeline()->enableDepthWriting(false);
	getPipeline()->setBlending(GraphicsAPI::ADDITIVE);

	// Finish the shader creation
	finish();
}
GBufferOutShaderNoDepth::~GBufferOutShaderNoDepth() { }

void GBufferOutShaderNoDepth::bind() {

	// Call parent to bind shaders
	Shader::bind();

}
