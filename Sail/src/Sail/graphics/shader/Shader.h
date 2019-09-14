#pragma once

#include "Sail/api/shader/ShaderPipeline.h"
#include <memory>
#include <string>

class Shader {
public:
	// Abstract classes used for compute shaders to specify its inputs and outputs
	class ComputeShaderInput {
	public:
		virtual ~ComputeShaderInput() {}
		unsigned int outputWidth = 100;
		unsigned int outputHeight = 100;
		unsigned int threadGroupCountX = 1;
		unsigned int threadGroupCountY = 1;
		unsigned int threadGroupCountZ = 1;
	};
	class ComputeShaderOutput {
	public:
		virtual ~ComputeShaderOutput() {}
	};
	struct ComputeSettings {
		bool usesCBV_SRV_UAV = false;
		unsigned int numInputTextures = 0;
		unsigned int numOutputTextures = 0;
		// TODO: expand settings to support more types
	};

public:
	Shader(const std::string& filename);
	virtual ~Shader();

	ShaderPipeline* getPipeline();

	// Graphics specific
	virtual void bind();
	virtual void setClippingPlane(const glm::vec4& clippingPlane) {};

	// Compute specific
	virtual const ComputeSettings* getComputeSettings() const { return nullptr; };
	virtual std::pair<std::string, Texture*> getComputeInputForIndex(Shader::ComputeShaderInput& input, unsigned int index) { return { "", nullptr }; };
	virtual Shader::ComputeShaderOutput* getComputeOutput() { return nullptr; };
	virtual RenderableTexture* getComputeOutputForIndex(Shader::ComputeShaderOutput& output, unsigned int index) { return nullptr; };

protected:
	void finish();
protected:
	// This is a raw pointer and not a smart pointer because reload() requires new (*) T() functionality
	ShaderPipeline* shaderPipeline;
private:
	bool m_finished;

};