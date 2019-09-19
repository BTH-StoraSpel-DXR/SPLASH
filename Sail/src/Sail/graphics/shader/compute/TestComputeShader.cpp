#include "pch.h"
#include "TestComputeShader.h"
#include "Sail/Application.h"

const Shader::ComputeSettings* TestComputeShader::getComputeSettings() const {
	return &m_settings;
}

std::pair<std::string, void*> TestComputeShader::getComputeInputForIndex(Shader::ComputeShaderInput& input, unsigned int index) {
	auto& thisInput = static_cast<TestComputeShader::Input&>(input);
	switch (index) {
	case 0:
		return {"input", thisInput.inputTexture};
	}
	Logger::Error("Tried to get compute input from unknown index - " + std::to_string(index));
	return {"", nullptr};
}

RenderableTexture* TestComputeShader::getComputeOutputForIndex(Shader::ComputeShaderOutput& output, unsigned int index) {
	auto& thisOutput = static_cast<TestComputeShader::Output&>(output);
	switch (index) {
	case 0:
		return thisOutput.outputTexture;
	}
	Logger::Error("Tried to get compute output from unknown index - " + std::to_string(index));
	return nullptr;
}

Shader::ComputeShaderOutput* TestComputeShader::getComputeOutput() {
	return &m_output;
}

TestComputeShader::TestComputeShader()
	: Shader("compute/ComputeTest.hlsl")
	, m_clippingPlaneHasChanged(false)
{
	// Finish the shader creation
	finish();

	// Specify dispatch requirements
	m_settings.usesCBV_SRV_UAV = true;
	m_settings.numInputTextures = 1;
	m_settings.numOutputTextures = 1;

	m_output.outputTexture = getPipeline()->getRenderableTexture("output");

}
TestComputeShader::~TestComputeShader() {
}