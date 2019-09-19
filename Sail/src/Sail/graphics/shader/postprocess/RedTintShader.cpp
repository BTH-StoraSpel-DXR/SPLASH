#include "pch.h"
#include "RedTintShader.h"
#include "Sail/Application.h"
#include "Sail/graphics/postprocessing/PostProcessPipeline.h"

const Shader::ComputeSettings* RedTintShader::getComputeSettings() const {
	return &m_settings;
}

std::pair<std::string, void*> RedTintShader::getComputeInputForIndex(Shader::ComputeShaderInput& input, unsigned int index) {
	auto& thisInput = static_cast<PostProcessPipeline::PostProcessInput&>(input);
	switch (index) {
	case 0:
		if (thisInput.inputTexture) {
			return { "input", thisInput.inputTexture };
		} else {
			return { "input", thisInput.inputRenderableTexture };
		}
	}
	Logger::Error("Tried to get compute input from unknown index - " + std::to_string(index));
	return { "", nullptr };
}

RenderableTexture* RedTintShader::getComputeOutputForIndex(Shader::ComputeShaderOutput& output, unsigned int index) {
	auto& thisOutput = static_cast<PostProcessPipeline::PostProcessOutput&>(output);
	switch (index) {
	case 0:
		return thisOutput.outputTexture;
	}
	Logger::Error("Tried to get compute output from unknown index - " + std::to_string(index));
	return nullptr;
}

Shader::ComputeShaderOutput* RedTintShader::getComputeOutput() {
	return m_output.get();
}

RedTintShader::RedTintShader()
	: Shader("compute/ComputeTest.hlsl")
{
	// Finish the shader creation
	finish();

	// Specify dispatch requirements
	m_settings.usesCBV_SRV_UAV = true;
	m_settings.numInputTextures = 1;
	m_settings.numOutputTextures = 1;

	m_output = std::make_unique<PostProcessPipeline::PostProcessOutput>();
	m_output->outputTexture = getPipeline()->getRenderableTexture("output");

}
RedTintShader::~RedTintShader() {}