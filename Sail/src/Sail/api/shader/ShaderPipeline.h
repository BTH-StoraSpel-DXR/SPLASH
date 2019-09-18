#pragma once

#include <memory>
//#include <d3d11.h>
//#include <d3dcompiler.h>
//#include <iostream>
//#include <glm/glm.hpp>
#include <glm/vec4.hpp>

#include "Sail/api/shader/ConstantBuffer.h"
#include "Sail/api/shader/Sampler.h"
//#include "Sail/graphics/geometry/Model.h"
#include "Sail/graphics/camera/Camera.h"
#include "Sail/utils/Utils.h"
#include "InputLayout.h"

class ShaderPipeline {
public:
	friend class Shader;
	static ShaderPipeline* CurrentlyBoundShader;
	static const std::string DEFAULT_SHADER_LOCATION;

public:
	static ShaderPipeline* Create(const std::string& filename);
	ShaderPipeline(const std::string& filename);
	virtual ~ShaderPipeline();

	// The following static methods are to be implemented in APIs
	virtual void bind(void* cmdList = nullptr) = 0;
	// filepath is used for include paths and error messages 
	virtual void* compileShader(const std::string& source, const std::string& filepath, ShaderComponent::BIND_SHADER shaderType) = 0;
	virtual void setTexture2D(const std::string& name, Texture* texture, void* cmdList = nullptr) = 0;

	virtual void updateCamera(Camera& cam) {};
	virtual void setClippingPlane(const glm::vec4& clippingPlane) {};


	InputLayout& getInputLayout();
	void* getVsBlob();
	const std::string& getName() const;

	void setCBufferVar(const std::string& name, const void* data, UINT size);
	bool trySetCBufferVar(const std::string& name, const void* data, UINT size);

protected:
	// Compiles shaders into blobs
	virtual void compile();
	// Called after the inputlayout is created
	virtual void finish();

	//void setComputeShaders(ID3D10Blob** blob, UINT numBlobs);

protected:
	std::unique_ptr<InputLayout> inputLayout;
	std::string filename;

	void* vsBlob; // Used for the input layout
	void* gsBlob;
	void* psBlob;
	void* dsBlob;
	void* hsBlob;


	struct ShaderResource {
		ShaderResource(const std::string& name, UINT slot)
			: name(name)
			, slot(slot)
		{}
		std::string name;
		UINT slot;
	};
	struct ShaderCBuffer {
		struct CBufferVariable {
			std::string name;
			UINT byteOffset;
		};
		ShaderCBuffer(std::vector<ShaderCBuffer::CBufferVariable>& vars, void* initData, UINT size, ShaderComponent::BIND_SHADER bindShader, UINT slot)
			: vars(vars)
		{
			cBuffer = std::unique_ptr<ShaderComponent::ConstantBuffer>(ShaderComponent::ConstantBuffer::Create(initData, size, bindShader, slot));
		}
		std::vector<CBufferVariable> vars;
		std::unique_ptr <ShaderComponent::ConstantBuffer> cBuffer;
	};
	struct ShaderSampler {
		ShaderSampler(ShaderResource res, Texture::ADDRESS_MODE adressMode, Texture::FILTER filter, ShaderComponent::BIND_SHADER bindShader, UINT slot)
			: res(res)
		{
			sampler = std::unique_ptr<ShaderComponent::Sampler>(ShaderComponent::Sampler::Create(adressMode, filter, bindShader, slot));

		}
		ShaderResource res;
		std::unique_ptr<ShaderComponent::Sampler> sampler;
	};
	struct ParsedData {
		bool hasVS = false, hasPS = false, hasGS = false, hasDS = false, hasHS = false;
		std::vector<ShaderCBuffer> cBuffers;
		std::vector<ShaderSampler> samplers;
		std::vector<ShaderResource> textures;
		void clear() {
			hasVS = false; hasPS = false; hasGS = false; hasDS = false; hasHS = false;
			cBuffers.clear();
			samplers.clear();
			textures.clear();
		}
	};
	ParsedData parsedData;

private:
	//std::vector<std::unique_ptr<ComputeShader>> m_css;
	//std::unique_ptr<Shader> m_shaders;


private:
	void parse(const std::string& source);
	void parseCBuffer(const std::string& source);
	void parseSampler(const char* source);
	void parseTexture(const char* source);
	std::string nextTokenAsName(const char* source, UINT& outTokenSize, bool allowArray = false) const;
	ShaderComponent::BIND_SHADER getBindShaderFromName(const std::string& name) const;

	//void setVertexShader(void* blob);
	//void setGeometryShader(void* blob);
	//void setPixelShader(void* blob);
	//void setDomainShader(void* blob);
	//void setHullShader(void* blob);

	UINT getSizeOfType(const std::string& typeName) const;

protected:
	UINT findSlotFromName(const std::string& name, const std::vector<ShaderResource>& resources) const;
};