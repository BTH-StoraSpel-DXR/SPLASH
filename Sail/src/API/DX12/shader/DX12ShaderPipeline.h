#pragma once

#include "../DX12API.h"
#include "Sail/api/shader/ShaderPipeline.h"
#include "DXILShaderCompiler.h"
#include "../resources/DX12ATexture.h"

class DX12ShaderPipeline : public ShaderPipeline {
public:
	DX12ShaderPipeline(const std::string& filename);
	~DX12ShaderPipeline();

	/*[deprecated]. Not thread safe. Use bind_new().*/
	virtual void bind(void* cmdList) override;
	virtual void bind_new(void* cmdList, int meshIndex);

	// Only used by compute shaders
	virtual void dispatch(unsigned int threadGroupCountX, unsigned int threadGroupCountY, unsigned int threadGroupCountZ, void* cmdList = nullptr) override;
	virtual void* compileShader(const std::string& source, const std::string& filepath, ShaderComponent::BIND_SHADER shaderType) override;
	virtual unsigned int setMaterial(Material* texture, void* cmdList);
	void checkBufferSizes(unsigned int nMeshes);
	virtual void setTexture2D(const std::string& name, Texture* texture, void* cmdList = nullptr) override;
	virtual void setTexture2D(const std::string& name, RenderableTexture* texture, void* cmdList = nullptr) override;

	void setDXTexture2D(DX12ATexture* dxTexture, ID3D12GraphicsCommandList4* dxCmdList);

	void setCBufferVar_new(const std::string& name, const void* data, UINT size, int meshIndex);
	bool trySetCBufferVar_new(const std::string& name, const void* data, UINT size, int meshIndex);


private:
	void createGraphicsPipelineState();
	void createComputePipelineState();

protected:
	virtual void compile() override;
	virtual void finish() override;

private:
	DX12API* m_context;

	static std::unique_ptr<DXILShaderCompiler> m_dxilCompiler; // Class Singleton
	wComPtr<ID3D12PipelineState> m_pipelineState;
};