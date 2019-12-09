#pragma once

#include "../DX12API.h"
#include "Sail/api/shader/ShaderPipeline.h"
#include "DXILShaderCompiler.h"
#include "../resources/DX12ATexture.h"

class DX12ShaderPipeline : public ShaderPipeline, public EventReceiver {
public:
	DX12ShaderPipeline(const std::string& filename);
	~DX12ShaderPipeline();

	bool onEvent(const Event& e) override;

	virtual void bind(void* cmdList) override;

	// Only used by compute shaders
	virtual void dispatch(unsigned int threadGroupCountX, unsigned int threadGroupCountY, unsigned int threadGroupCountZ, void* cmdList = nullptr) override;
	virtual void* compileShader(const std::string& source, const std::string& filepath, ShaderComponent::BIND_SHADER shaderType) override;

	virtual unsigned int setMaterial(PBRMaterial* texture, void* cmdList);
	void checkBufferSizes(unsigned int nMeshes);
	virtual void setTexture2D(const std::string& name, Texture* texture, void* cmdList = nullptr) override;
	virtual void setTexture2D(const std::string& name, RenderableTexture* texture, void* cmdList = nullptr) override;

	void setDXTexture2D(DX12ATexture* dxTexture, ID3D12GraphicsCommandList4* dxCmdList);

	virtual bool trySetCBufferVar(const std::string& name, const void* data, UINT size) override;
	virtual bool trySetStructBufferVar(const std::string& name, const void* data, UINT numElements) override;

	void setRenderTargetFormat(unsigned int rtIndex, DXGI_FORMAT format);

	// Call this after each mesh/instance
	// Internally updates meshIndex used to place multiple instances in a single cbuffer
	void instanceFinished();
	
private:
	void createGraphicsPipelineState();
	void createComputePipelineState();
	unsigned int getMeshIndex();
	// Resets internal meshIndex
	void newFrame();

protected:
	virtual void compile() override;
	virtual void finish() override;

private:
	std::atomic_uint m_meshIndex[2];
	DX12API* m_context;

	std::unordered_map<unsigned int, DXGI_FORMAT> m_rtFormats;
	static std::unique_ptr<DXILShaderCompiler> m_dxilCompiler; // Class Singleton
	wComPtr<ID3D12PipelineState> m_pipelineState;
};