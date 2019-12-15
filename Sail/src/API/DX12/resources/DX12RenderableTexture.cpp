#include "pch.h"
#include "DX12RenderableTexture.h"
#include "Sail/Application.h"
#include "Sail/api/Window.h"
#include "../DX12Utils.h"

RenderableTexture* RenderableTexture::Create(unsigned int width, unsigned int height, const std::string& name, Texture::FORMAT format, bool createDepthStencilView, bool createOnlyDSV, unsigned int arraySize, bool singleBuffer) {
	return SAIL_NEW DX12RenderableTexture(1, width, height, format, createDepthStencilView, createOnlyDSV, singleBuffer, 0U, 0U, name, arraySize); // TODO: change singleBuffer back when issues with soft shadows are fixed!!
}

DX12RenderableTexture::DX12RenderableTexture(UINT aaSamples, unsigned int width, unsigned int height, Texture::FORMAT format, bool createDepthStencilView, bool createOnlyDSV, bool singleBuffer, UINT bindFlags, UINT cpuAccessFlags, const std::string& name, unsigned int arraySize)
	: m_width(width)
	, m_height(height)
	, m_cpuRtvDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, Application::getInstance()->getAPI<DX12API>()->getNumGPUBuffers())
	, m_cpuDsvDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, Application::getInstance()->getAPI<DX12API>()->getNumGPUBuffers())
	, m_hasDepthTextures(createDepthStencilView)
	, m_arraySize(arraySize)
{
	isRenderableTex = true;
	context = Application::getInstance()->getAPI<DX12API>();

	m_name = name;

	m_format = convertFormat(format);

	m_numSwapBuffers = context->getNumGPUBuffers();
	if (singleBuffer) {
		// RenderableTextures are not multi-buffered, consecutive frames will use the same render textures.
		useOneResource = true;
		m_numSwapBuffers = 1;
	}

	m_rtvHeapCDHs.resize(m_numSwapBuffers);
	m_dsvHeapCDHs.resize(m_numSwapBuffers);
	if (m_hasDepthTextures) {
		m_depthStencilBuffers.resize(m_numSwapBuffers);
	}
	for (unsigned int i = 0; i < m_numSwapBuffers; i++) {
		m_rtvHeapCDHs[i] = m_cpuRtvDescHeap.getCPUDescriptorHandleForIndex(i);
		m_dsvHeapCDHs[i] = m_cpuDsvDescHeap.getCPUDescriptorHandleForIndex(i);
	}
	createTextures();
}

DX12RenderableTexture::~DX12RenderableTexture() {

}

void DX12RenderableTexture::begin(void* cmdList) {
	auto* dxCmdList = static_cast<ID3D12GraphicsCommandList4*>(cmdList);

	//transitionStateTo(dxCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

	const D3D12_CPU_DESCRIPTOR_HANDLE* dsvCdh = (m_hasDepthTextures) ? &m_dsvHeapCDHs[getSwapIndex()] : &context->getDepthStencilViewCDH();
	dxCmdList->OMSetRenderTargets(1, &m_rtvHeapCDHs[getSwapIndex()], true, dsvCdh);

	dxCmdList->RSSetViewports(1, context->getViewport());
	dxCmdList->RSSetScissorRects(1, context->getScissorRect());
}

void DX12RenderableTexture::end(void* cmdList) {
	// Does nothing
}

void DX12RenderableTexture::clear(const glm::vec4& color, void* cmdList) {
	auto* dxCmdList = static_cast<ID3D12GraphicsCommandList4*>(cmdList);
	// Clear
	FLOAT clearColor[4];
	clearColor[0] = color.r;
	clearColor[1] = color.g;
	clearColor[2] = color.b;
	clearColor[3] = color.a;

	// Resource state must be set to render target before clearing
	transitionStateTo(dxCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	dxCmdList->ClearRenderTargetView(m_rtvHeapCDHs[getSwapIndex()], clearColor, 0, nullptr);
	if (m_hasDepthTextures) {
		dxCmdList->ClearDepthStencilView(m_dsvHeapCDHs[getSwapIndex()], D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}
}

void DX12RenderableTexture::changeFormat(Texture::FORMAT newFormat) {
	auto newDxgiFormat = convertFormat(newFormat);
	if (m_format == newDxgiFormat) {
		return;
	}
	m_format = newDxgiFormat;
	createTextures();
}

void DX12RenderableTexture::resize(int width, int height) {
	if (width == m_width && height == m_height) {
		return;
	}
	m_width = width;
	m_height = height;
	createTextures();
}

ID3D12Resource* DX12RenderableTexture::getResource(int frameIndex) const {
	unsigned int i = (frameIndex == -1 || m_numSwapBuffers == 1) ? getSwapIndex() : frameIndex;
	return textureDefaultBuffers[i].Get();
}

ID3D12Resource* DX12RenderableTexture::getDepthResource() const {
	return m_depthStencilBuffers[getSwapIndex()].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12RenderableTexture::getDepthSrvCDH(int frameIndex) const {
	assert(m_hasDepthTextures); // Tried to get depth srv without a valid depth stencil resource
	unsigned int i = (frameIndex == -1 || m_numSwapBuffers == 1) ? getSwapIndex() : frameIndex;
	return depthSrvHeapCDHs[i];
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12RenderableTexture::getRtvCDH(int frameIndex) const {
	unsigned int i = (frameIndex == -1 || m_numSwapBuffers == 1) ? getSwapIndex() : frameIndex;
	return m_rtvHeapCDHs[i];
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12RenderableTexture::getDsvCDH(int frameIndex) const {
	unsigned int i = (frameIndex == -1 || m_numSwapBuffers == 1) ? getSwapIndex() : frameIndex;
	return m_dsvHeapCDHs[i];
}

DXGI_FORMAT DX12RenderableTexture::convertFormat(Texture::FORMAT format) const {
	DXGI_FORMAT dxgiFormat;
	switch (format) {
	case Texture::R8:
		dxgiFormat = DXGI_FORMAT_R8_UNORM;
		break;
	case Texture::R8G8:
		dxgiFormat = DXGI_FORMAT_R8G8_UNORM;
		break;
	case Texture::R8G8B8A8:
		dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case Texture::R16_FLOAT:
		dxgiFormat = DXGI_FORMAT_R16_FLOAT;
		break;
	case Texture::R16G16_FLOAT:
		dxgiFormat = DXGI_FORMAT_R16G16_FLOAT;
		break;
	case Texture::R16G16B16A16_FLOAT:
		dxgiFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
		break;
	case Texture::R32G32B32A32_FLOAT:
		dxgiFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		break;
	default:
		dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	}
	return dxgiFormat;
}

void DX12RenderableTexture::createTextures() {	
	context->waitForGPU();
	for (unsigned int i = 0; i < m_numSwapBuffers; i++) {
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.Format = m_format;
		textureDesc.Width = m_width;
		textureDesc.Height = m_height;
		textureDesc.DepthOrArraySize = m_arraySize;
		textureDesc.MipLevels = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		D3D12_CLEAR_VALUE clearValue = { textureDesc.Format, { 0.01f, 0.01f, 0.01f, 1.0f } };

		state[i] = D3D12_RESOURCE_STATE_COMMON;
		// A texture rarely updates its data, if at all, so it is stored in a default heap
		ThrowIfFailed(context->getDevice()->CreateCommittedResource(&DX12Utils::sDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &textureDesc, state[i], &clearValue, IID_PPV_ARGS(&textureDefaultBuffers[i])));

		std::wstring stemp = std::wstring(m_name.begin(), m_name.end());
		textureDefaultBuffers[i]->SetName(stemp.c_str());

		// Create a shader resource view (descriptor that points to the texture and describes it)
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		if (m_arraySize > 1) {
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.ArraySize = m_arraySize;
			srvDesc.Texture2DArray.MipLevels = 1;
		} else {
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;
		}
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureDesc.Format;
		context->getDevice()->CreateShaderResourceView(textureDefaultBuffers[i].Get(), &srvDesc, srvHeapCDHs[i]);

		// Create a unordered access view
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		if (m_arraySize > 1) {
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			uavDesc.Texture2DArray.ArraySize = m_arraySize;
		} else {
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		}
		uavDesc.Format = textureDesc.Format;
		context->getDevice()->CreateUnorderedAccessView(textureDefaultBuffers[i].Get(), nullptr, &uavDesc, uavHeapCDHs[i]);

		// Create a render target view
		context->getDevice()->CreateRenderTargetView(textureDefaultBuffers[i].Get(), nullptr, m_rtvHeapCDHs[i]);
	}

	if (m_hasDepthTextures) {
		createDepthTextures();
	}
}

void DX12RenderableTexture::createDepthTextures() {
	for (unsigned int i = 0; i < m_numSwapBuffers; i++) {

		D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
		depthOptimizedClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
		depthOptimizedClearValue.DepthStencil.Stencil = 0;

		D3D12_RESOURCE_DESC bufferDesc{};
		bufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		bufferDesc.Width = Application::getInstance()->getWindow()->getWindowWidth();
		bufferDesc.Height = Application::getInstance()->getWindow()->getWindowHeight();
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.MipLevels = 0;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.SampleDesc.Quality = 0;
		bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		context->getDevice()->CreateCommittedResource(
			&DX12Utils::sDefaultHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthOptimizedClearValue,
			IID_PPV_ARGS(&m_depthStencilBuffers[i])
		);
		m_depthStencilBuffers[i]->SetName(L"Depth/Stencil Resource Buffer");

		// Create DSV
		D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
		depthStencilDesc.Format = bufferDesc.Format;
		depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;
		context->getDevice()->CreateDepthStencilView(m_depthStencilBuffers[i].Get(), &depthStencilDesc, m_dsvHeapCDHs[i]);

		// Create a shader resource view for the depth stencil
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		context->getDevice()->CreateShaderResourceView(m_depthStencilBuffers[i].Get(), &srvDesc, depthSrvHeapCDHs[i]);
	}
}

unsigned int DX12RenderableTexture::getSwapIndex() const {
	return (m_numSwapBuffers == 1) ? 0 : context->getSwapIndex();
}
