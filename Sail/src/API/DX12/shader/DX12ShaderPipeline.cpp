#include "pch.h"
#include "DX12ShaderPipeline.h"
#include "Sail/Application.h"
#include "DX12InputLayout.h"
#include "../DX12API.h"
#include "DX12ConstantBuffer.h"
#include "DX12StructuredBuffer.h"
#include "../resources/DX12Texture.h"
#include "../resources/DX12RenderableTexture.h"

std::unique_ptr<DXILShaderCompiler> DX12ShaderPipeline::m_dxilCompiler = nullptr;

ShaderPipeline* ShaderPipeline::Create(const std::string& filename) {
	return SAIL_NEW DX12ShaderPipeline(filename);
}

DX12ShaderPipeline::DX12ShaderPipeline(const std::string& filename)
	: ShaderPipeline(filename) 
	, m_numRenderTargets(1)
	, m_enableDepth(true)
	, m_enableBlending(false)
{
	m_context = Application::getInstance()->getAPI<DX12API>();

	if (!m_dxilCompiler) {
		m_dxilCompiler = std::make_unique<DXILShaderCompiler>();
		m_dxilCompiler->init();
	}
}

DX12ShaderPipeline::~DX12ShaderPipeline() {
	m_context->waitForGPU();
}

/*[deprecated]*/
void DX12ShaderPipeline::bind(void* cmdList) {
	assert(false);/*[deprecated]*/
}

void DX12ShaderPipeline::bind_new(void* cmdList, int meshIndex) {
	if (!m_pipelineState)
		Logger::Error("Tried to bind DX12PipelineState before the DirectX PipelineStateObject has been created!");
	auto* dxCmdList = static_cast<ID3D12GraphicsCommandList4*>(cmdList);

	for (auto& it : parsedData.cBuffers) {
		auto* dxCBuffer = static_cast<ShaderComponent::DX12ConstantBuffer*>(it.cBuffer.get());
		dxCBuffer->bind_new(cmdList, meshIndex, csBlob != nullptr);
	}
	for (auto& it : parsedData.structuredBuffers) {
		auto* dxSBuffer = static_cast<ShaderComponent::DX12StructuredBuffer*>(it.sBuffer.get());
		dxSBuffer->bind_new(cmdList, meshIndex);
	}
	for (auto& it : parsedData.samplers) {
		it.sampler->bind();
	}
	for (auto& it : parsedData.renderableTextures) {
		auto* dxRendTexture = static_cast<DX12RenderableTexture*>(it.renderableTexture.get());
		dxRendTexture->transitionStateTo(dxCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		// Copy descriptor heap to the GPU bound heap
		m_context->getDevice()->CopyDescriptorsSimple(1, m_context->getComputeGPUDescriptorHeap()->getNextCPUDescriptorHandle(), dxRendTexture->getUavCDH(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	// Set input layout as active
	inputLayout->bind();

	// TODO: move this to somewhere where it is more efficient
	dxCmdList->SetPipelineState(m_pipelineState.Get());
}

void DX12ShaderPipeline::dispatch(unsigned int threadGroupCountX, unsigned int threadGroupCountY, unsigned int threadGroupCountZ, void* cmdList) {
	auto* dxCmdList = static_cast<ID3D12GraphicsCommandList4*>(cmdList);
	dxCmdList->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void* DX12ShaderPipeline::compileShader(const std::string& source, const std::string& filepath, ShaderComponent::BIND_SHADER shaderType) {

	// TODO: make this work
//	DXILShaderCompiler::Desc shaderDesc;
//	
//	switch (shaderType) {
//	case ShaderComponent::VS:
//		shaderDesc.entryPoint = L"VSMain";
//		shaderDesc.targetProfile = L"vs_6_0";
//		break;
//	case ShaderComponent::PS:
//		shaderDesc.entryPoint = L"PSMain";
//		shaderDesc.targetProfile = L"ps_6_0";
//		break;
//	case ShaderComponent::GS:
//		shaderDesc.entryPoint = L"GSMain";
//		shaderDesc.targetProfile = L"gs_6_0";
//		break;
//	case ShaderComponent::CS:
//		shaderDesc.entryPoint = L"CSMain";
//		shaderDesc.targetProfile = L"cs_6_0";
//		break;
//	case ShaderComponent::DS:
//		shaderDesc.entryPoint = L"DSMain";
//		shaderDesc.targetProfile = L"ds_6_0";
//		break;
//	case ShaderComponent::HS:
//		shaderDesc.entryPoint = L"HSMain";
//		shaderDesc.targetProfile = L"hs_6_0";
//		break;
//	}
//	
//#ifdef _DEBUG
//	shaderDesc.compileArguments.push_back(L"/Zi"); // Debug info
//#endif
//	shaderDesc.compileArguments.push_back(L"/Gis"); // Declare all variables and values as precise
//	shaderDesc.source = source.c_str();
//	shaderDesc.sourceSize = source.length();
//	auto wfilepath = std::wstring(filepath.begin(), filepath.end());
//	shaderDesc.filePath = wfilepath.c_str();
//
//	IDxcBlob* pShaders = nullptr;
//	ThrowIfFailed(m_dxilCompiler->compile(&shaderDesc, &pShaders));

	// "Old" compilation

	ID3DBlob* pShaders = nullptr;
	ID3DBlob* errorBlob = nullptr;
	UINT flags = 0;
#if defined( DEBUG ) || defined( _DEBUG )
	flags |= D3DCOMPILE_DEBUG;
	flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
	flags |= D3DCOMPILE_ALL_RESOURCES_BOUND;
#endif
	HRESULT hr;
	switch (shaderType) {
	case ShaderComponent::VS:
		hr = D3DCompile(source.c_str(), source.length(), filepath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_0", flags, 0, &pShaders, &errorBlob);
		break;
	case ShaderComponent::GS:
		hr = D3DCompile(source.c_str(), source.length(), filepath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "GSMain", "gs_5_0", flags, 0, &pShaders, &errorBlob);
		break;
	case ShaderComponent::PS:
		hr = D3DCompile(source.c_str(), source.length(), filepath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_0", flags, 0, &pShaders, &errorBlob);
		break;
	case ShaderComponent::CS:
		hr = D3DCompile(source.c_str(), source.length(), filepath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSMain", "cs_5_0", flags, 0, &pShaders, &errorBlob);
		break;
	}

	if (FAILED(hr)) {
		// Print shader compilation error
		if (errorBlob) {
			char* msg = (char*)(errorBlob->GetBufferPointer());
			std::stringstream ss;
			ss << "Failed to compile shader (" << filepath << ")\n";
			for (size_t i = 0; i < errorBlob->GetBufferSize(); i++) {
				ss << msg[i];
			}
			OutputDebugStringA(ss.str().c_str());
			OutputDebugStringA("\n");
			MessageBoxA(0, ss.str().c_str(), "", 0);
			errorBlob->Release();
		}
		if (pShaders) {
			pShaders->Release();
		}
		ThrowIfFailed(hr);
	}

	return pShaders;

}

void DX12ShaderPipeline::setTexture2D(const std::string& name, RenderableTexture* texture, void* cmdList) {
	auto* dxCmdList = static_cast<ID3D12GraphicsCommandList4*>(cmdList);
	DX12RenderableTexture* dxTexture = static_cast<DX12RenderableTexture*>(texture);

	setDXTexture2D(dxTexture, dxCmdList);
}

void DX12ShaderPipeline::setTexture2D(const std::string& name, Texture* texture, void* cmdList) {
	auto* dxCmdList = static_cast<ID3D12GraphicsCommandList4*>(cmdList);
	DX12Texture* dxTexture = static_cast<DX12Texture*>(texture);
	if (!dxTexture->hasBeenInitialized()) {
		assert(false); // Is this used?
		//dxTexture->initBuffers(dxCmdList);
	}

	setDXTexture2D(dxTexture, dxCmdList);
}

void DX12ShaderPipeline::setDXTexture2D(DX12ATexture* dxTexture, ID3D12GraphicsCommandList4* dxCmdList) {
	// Copy texture resource view to the gpu heap
	if (csBlob) {
		dxTexture->transitionStateTo(dxCmdList, D3D12_RESOURCE_STATE_GENERIC_READ);
		// Use compute heap
		m_context->getDevice()->CopyDescriptorsSimple(1, m_context->getComputeGPUDescriptorHeap()->getNextCPUDescriptorHandle(), dxTexture->getSrvCDH(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	} else {
		dxTexture->transitionStateTo(dxCmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		// Use main/graphics heap
		m_context->getDevice()->CopyDescriptorsSimple(1, m_context->getMainGPUDescriptorHeap()->getNextCPUDescriptorHandle(), dxTexture->getSrvCDH(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
}

unsigned int DX12ShaderPipeline::setMaterial(PBRMaterial* material, void* cmdList) {
	const PBRMaterial::PBRSettings& ps = material->getPBRSettings();
	int nTextures = 0;
	DX12Texture* textures[3];
	if (ps.hasAlbedoTexture) {
		textures[nTextures] = static_cast<DX12Texture*>(material->getTexture(nTextures));
		nTextures++;
	}
	if (ps.hasNormalTexture) {
		textures[nTextures] = static_cast<DX12Texture*>(material->getTexture(nTextures));
		nTextures++;
	}
	if (ps.hasMetalnessRoughnessAOTexture) {
		textures[nTextures] = static_cast<DX12Texture*>(material->getTexture(nTextures));
		nTextures++;
	}

	unsigned int indexStart = m_context->getMainGPUDescriptorHeap()->getAndStepIndex(nTextures);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_context->getMainGPUDescriptorHeap()->getCPUDescriptorHandleForIndex(indexStart);

	for (int i = 0; i < nTextures; i++) {
		if (!textures[i]->hasBeenInitialized()) {
			textures[i]->initBuffers(static_cast<ID3D12GraphicsCommandList4*>(cmdList), i);
		}

		textures[i]->transitionStateTo(static_cast<ID3D12GraphicsCommandList4*>(cmdList), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_context->getDevice()->CopyDescriptorsSimple(1, handle, textures[i]->getSrvCDH(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		handle.ptr += m_context->getMainGPUDescriptorHeap()->getDescriptorIncrementSize();
	}

	return indexStart;
}

/*
	Temp fix to expand constant buffers if the scene contain to many objects.
*/
void DX12ShaderPipeline::checkBufferSizes(unsigned int nMeshes) {
	for (auto& it : parsedData.cBuffers) {
		static_cast<ShaderComponent::DX12ConstantBuffer*>(it.cBuffer.get())->checkBufferSize(nMeshes);
	}
}

// TODO: size isnt really needed, can be read from the byteOffset of the next var
void DX12ShaderPipeline::setCBufferVar_new(const std::string& name, const void* data, UINT size, int meshIndex) {
	bool success = trySetCBufferVar_new(name, data, size, meshIndex);
	if (!success)
		Logger::Warning("Tried to set CBuffer variable that did not exist (" + name + ")");
}

bool DX12ShaderPipeline::trySetCBufferVar_new(const std::string& name, const void* data, UINT size, int meshIndex) {
	for (auto& it : parsedData.cBuffers) {
		for (auto& var : it.vars) {
			if (var.name == name) {
				ShaderComponent::DX12ConstantBuffer& cbuffer = static_cast<ShaderComponent::DX12ConstantBuffer&>(*it.cBuffer.get());
				cbuffer.updateData_new(data, size, meshIndex, var.byteOffset);
				return true;
			}
		}
	}
	return false;
}

void DX12ShaderPipeline::setNumRenderTargets(unsigned int numRenderTargets) { 
	m_numRenderTargets = numRenderTargets;
}

void DX12ShaderPipeline::enableDepthStencil(bool enable) {
	m_enableDepth = enable;
}

void DX12ShaderPipeline::enableAlphaBlending(bool enable) {
	m_enableBlending = enable;
}

void DX12ShaderPipeline::compile() {
	ShaderPipeline::compile();
}

void DX12ShaderPipeline::createGraphicsPipelineState() {
	auto vsD3DBlob = static_cast<ID3DBlob*>(vsBlob);
	auto psD3DBlob = static_cast<ID3DBlob*>(psBlob);
	auto gsD3DBlob = static_cast<ID3DBlob*>(gsBlob);
	auto dsD3DBlob = static_cast<ID3DBlob*>(dsBlob);
	auto hsD3DBlob = static_cast<ID3DBlob*>(hsBlob);
	auto csD3DBlob = static_cast<ID3DBlob*>(csBlob);
	
	////// Pipline State //////
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsd = {};

	// Specify pipeline stages
	gpsd.pRootSignature = m_context->getGlobalRootSignature();
	gpsd.InputLayout = static_cast<DX12InputLayout*>(inputLayout.get())->getDesc();
	gpsd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	if (vsBlob) {
		gpsd.VS.pShaderBytecode = reinterpret_cast<void*>(vsD3DBlob->GetBufferPointer());
		gpsd.VS.BytecodeLength = vsD3DBlob->GetBufferSize();
	}
	if (psBlob) {
		gpsd.PS.pShaderBytecode = reinterpret_cast<void*>(psD3DBlob->GetBufferPointer());
		gpsd.PS.BytecodeLength = psD3DBlob->GetBufferSize();
	}
	if (gsBlob) {
		gpsd.GS.pShaderBytecode = reinterpret_cast<void*>(gsD3DBlob->GetBufferPointer());
		gpsd.GS.BytecodeLength = gsD3DBlob->GetBufferSize();
	}
	if (dsBlob) {
		gpsd.DS.pShaderBytecode = reinterpret_cast<void*>(dsD3DBlob->GetBufferPointer());
		gpsd.DS.BytecodeLength = dsD3DBlob->GetBufferSize();
	}
	if (hsBlob) {
		gpsd.HS.pShaderBytecode = reinterpret_cast<void*>(hsD3DBlob->GetBufferPointer());
		gpsd.HS.BytecodeLength = hsD3DBlob->GetBufferSize();
	}

	// Specify render target and depthstencil usage
	for (unsigned int i = 0; i < m_numRenderTargets; i++) {
		gpsd.RTVFormats[i] = DXGI_FORMAT_R8G8B8A8_UNORM;
	}
	gpsd.NumRenderTargets = m_numRenderTargets;

	gpsd.SampleDesc.Count = 1;
	gpsd.SampleDesc.Quality = 0;
	gpsd.SampleMask = UINT_MAX;

	// Specify rasterizer behaviour
	if (wireframe) {
		gpsd.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	}
	else {
		gpsd.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	}

	if (cullMode == GraphicsAPI::Culling::NO_CULLING) {
		gpsd.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	} 
	else if (cullMode == GraphicsAPI::Culling::FRONTFACE) {
		gpsd.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
	}
	else if (cullMode == GraphicsAPI::Culling::BACKFACE) {
		gpsd.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	}

	//Specify blend descriptions.
	D3D12_RENDER_TARGET_BLEND_DESC defaultRTdesc;
	if (m_enableBlending) {
		defaultRTdesc = {
			true, false,
			D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_ONE, D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ZERO, D3D12_BLEND_ONE, D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL
		};
	} else {
		defaultRTdesc = {
			false, false,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL
		};
	}
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsd.BlendState.RenderTarget[i] = defaultRTdesc;

	// Specify depth stencil state descriptor.
	D3D12_DEPTH_STENCIL_DESC dsDesc{};
	dsDesc.DepthEnable = m_enableDepth;
	dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	dsDesc.StencilEnable = FALSE;
	dsDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	dsDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	dsDesc.FrontFace = defaultStencilOp;
	dsDesc.BackFace = defaultStencilOp;

	gpsd.DepthStencilState = dsDesc;
	gpsd.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	ThrowIfFailed(m_context->getDevice()->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(&m_pipelineState)));
}

void DX12ShaderPipeline::createComputePipelineState() {
	auto csD3DBlob = static_cast<ID3DBlob*>(csBlob);

	////// Pipline State //////
	D3D12_COMPUTE_PIPELINE_STATE_DESC cpsd = {};

	// Specify pipeline stages
	cpsd.pRootSignature = m_context->getGlobalRootSignature();
	cpsd.CS.pShaderBytecode = reinterpret_cast<void*>(csD3DBlob->GetBufferPointer());;
	cpsd.CS.BytecodeLength = csD3DBlob->GetBufferSize();

	ThrowIfFailed(m_context->getDevice()->CreateComputePipelineState(&cpsd, IID_PPV_ARGS(&m_pipelineState)));
}

void DX12ShaderPipeline::finish() {

	// Create a compute pipeline state if it has a compute shader
	if (csBlob) {
		createComputePipelineState();
	} else {
		createGraphicsPipelineState();
	}

}
