#include "pch.h"
#include "BlendShader.h"
#include "Sail/Application.h"
#include "Sail/graphics/postprocessing/PostProcessPipeline.h"

const Shader::ComputeSettings* BlendShader::getComputeSettings() const {
	return &m_settings;
}

std::pair<std::string, void*> BlendShader::getComputeInputForIndex(Shader::ComputeShaderInput& input, unsigned int index) {
	auto& thisInput = static_cast<PostProcessPipeline::PostProcessInput&>(input);
	switch (index) {
	case 0:
		if (thisInput.inputTexture) {
			return { "input", thisInput.inputTexture };
		} else {
			return { "input", thisInput.inputRenderableTexture };
		}
	case 1:
		if (thisInput.inputTextureTwo) {
			return { "input", thisInput.inputTextureTwo };
		} else {
			return { "input", thisInput.inputRenderableTextureTwo };
		}
	}
	SAIL_LOG_ERROR("Tried to get compute input from unknown index - " + std::to_string(index));
	return { "", nullptr };
}

RenderableTexture* BlendShader::getComputeOutputForIndex(Shader::ComputeShaderOutput& output, unsigned int index) {
	auto& thisOutput = static_cast<PostProcessPipeline::PostProcessOutput&>(output);
	switch (index) {
	case 0:
		return thisOutput.outputTexture;
	}
	SAIL_LOG_ERROR("Tried to get compute output from unknown index - " + std::to_string(index));
	return nullptr;
}

Shader::ComputeShaderOutput* BlendShader::getComputeOutput() {
	return m_output.get();
}

BlendShader::BlendShader()
	: Shader("postprocess/BlendShader.hlsl")
{
	// Finish the shader creation
	finish();

	// Specify dispatch requirements
	m_settings.usesCBV_SRV_UAV = true;
	m_settings.numInputTextures = 2;
	m_settings.numOutputTextures = 1;

	// Compute shader runs 256 x and y threads, therefore divide resolution by that when dispatching
	m_settings.threadGroupXScale = 1.f / 256.f;

	m_output = std::make_unique<PostProcessPipeline::PostProcessOutput>();
	m_output->outputTexture = getPipeline()->getRenderableTexture("output");

}
BlendShader::~BlendShader() {}