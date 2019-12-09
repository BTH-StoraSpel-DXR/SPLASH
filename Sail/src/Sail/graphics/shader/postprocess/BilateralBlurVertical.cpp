#include "pch.h"
#include "BilateralBlurVertical.h"
#include "Sail/Application.h"

BilateralBlurVertical::BilateralBlurVertical()
	: Shader("postprocess/BilateralBlurVertical.hlsl") {
	// Finish the shader creation
	finish();

	// Specify dispatch requirements
	// These are really a lie, but set to this to allow manual descriptor handling. See DX12RaytracingRenderer::runDenoising()
	m_settings.usesCBV_SRV_UAV = false;
	m_settings.numInputTextures = 0;
	m_settings.numOutputTextures = 0;

	// Compute shader runs 256 x threads, therefore divide resolution by that when dispatching
	m_settings.threadGroupYScale = 1.f / 256.f;

	m_output = std::make_unique<PostProcessPipeline::PostProcessOutput>();
}

BilateralBlurVertical::~BilateralBlurVertical() {}

const Shader::ComputeSettings* BilateralBlurVertical::getComputeSettings() const {
	return &m_settings;
}

std::pair<std::string, void*> BilateralBlurVertical::getComputeInputForIndex(Shader::ComputeShaderInput& input, unsigned int index) {
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

RenderableTexture* BilateralBlurVertical::getComputeOutputForIndex(Shader::ComputeShaderOutput& output, unsigned int index) {
	auto& thisOutput = static_cast<PostProcessPipeline::PostProcessOutput&>(output);
	switch (index) {
	case 0:
		return thisOutput.outputTexture;
	}
	Logger::Error("Tried to get compute output from unknown index - " + std::to_string(index));
	return nullptr;
}

Shader::ComputeShaderOutput* BilateralBlurVertical::getComputeOutput() {
	return m_output.get();
}
