#pragma once

#include "Sail/graphics/shader/Shader.h"
#include "Sail/graphics/postprocessing/PostProcessPipeline.h"

class BilateralBlurHorizontal : public Shader {
public:
	BilateralBlurHorizontal();
	~BilateralBlurHorizontal();

	virtual const Shader::ComputeSettings* getComputeSettings() const override;
	virtual std::pair<std::string, void*> getComputeInputForIndex(Shader::ComputeShaderInput& input, unsigned int index) override;
	virtual RenderableTexture* getComputeOutputForIndex(Shader::ComputeShaderOutput& output, unsigned int index) override;
	virtual Shader::ComputeShaderOutput* getComputeOutput() override;

private:
	Shader::ComputeSettings m_settings;
	std::unique_ptr<PostProcessPipeline::PostProcessOutput> m_output;

private:

};