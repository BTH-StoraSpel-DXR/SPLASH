#pragma once

#include <d3d11.h>
#include <glm/glm.hpp>
#include "Sail/api/shader/ShaderPipeline.h"
#include "Sail/graphics/shader/Shader.h"
#include "../../geometry/Material.h"

class TestComputeShader : public Shader {
public:
	class Input : public Shader::ComputeShaderInput {
	public:
		Texture* inputTexture;
	};
	class Output : public Shader::ComputeShaderOutput {
	public:
		RenderableTexture* outputTexture;
	};


public:
	TestComputeShader();
	~TestComputeShader();

	virtual const Shader::ComputeSettings* getComputeSettings() const override;
	virtual std::pair<std::string, void*> getComputeInputForIndex(Shader::ComputeShaderInput& input, unsigned int index) override;
	virtual RenderableTexture* getComputeOutputForIndex(Shader::ComputeShaderOutput& output, unsigned int index) override;
	virtual Shader::ComputeShaderOutput* getComputeOutput() override;

private:
	bool m_clippingPlaneHasChanged;
	Shader::ComputeSettings m_settings;
	Output m_output;

};
