#include "pch.h"
#include "GuiShader.h"
#include "API/DX12/shader/DX12ShaderPipeline.h"


GuiShader::GuiShader()
	: Shader("gui/GuiShader.hlsl")
{
	shaderPipeline->getInputLayout().pushVec2(InputLayout::POSITION2D, "POSITION", 0);
	shaderPipeline->getInputLayout().pushVec2(InputLayout::TEXCOORD, "TEXCOORD", 0);
	shaderPipeline->getInputLayout().create(shaderPipeline->getVsBlob());

	shaderPipeline->enableDepthStencil(false);
	shaderPipeline->setBlending(GraphicsAPI::ALPHA);

	// Finish the shader creation
	finish();
}

GuiShader::~GuiShader() {
}

void GuiShader::bind() {
	Shader::bind();
}