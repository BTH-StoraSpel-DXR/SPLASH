#include "pch.h"
#include "DX12RenderableTexture.h"
#include "Sail/Application.h"
#include "Sail/api/Window.h"
#include "../DX12Utils.h"

RenderableTexture* RenderableTexture::Create(unsigned int width, unsigned int height, const std::string& name) {
	return SAIL_NEW DX12RenderableTexture(1, width, height, true, false, 0U, 0U, name);
}

DX12RenderableTexture::DX12RenderableTexture(UINT aaSamples, unsigned int width, unsigned int height, bool createDepthStencilView, bool createOnlyDSV, UINT bindFlags, UINT cpuAccessFlags, const std::string& name)
	: m_width(width)
	, m_height(height)
	, m_cpuRtvDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, Application::getInstance()->getAPI<DX12API>()->getNumSwapBuffers())
	, m_cpuDsvDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, Application::getInstance()->getAPI<DX12API>()->getNumSwapBuffers())
	, m_hasDepthTextures(createDepthStencilView)
{
	isRenderableTex = true;
	context = Application::getInstance()->getAPI<DX12API>();

	const auto& numSwapBuffers = context->getNumSwapBuffers();
	m_rtvHeapCDHs.resize(numSwapBuffers);
	m_dsvHeapCDHs.resize(numSwapBuffers);
	if (m_hasDepthTextures) {
		m_depthStencilBuffers.resize(numSwapBuffers);
	}
	for (unsigned int i = 0; i < numSwapBuffers; i++) {
		m_rtvHeapCDHs[i] = m_cpuRtvDescHeap.getCPUDescriptorHandleForIndex(i);
		m_dsvHeapCDHs[i] = m_cpuDsvDescHeap.getCPUDescriptorHandleForIndex(i);
	}
	createTextures();
	renameBuffer(name);
}

DX12RenderableTexture::~DX12RenderableTexture() {

}

void DX12RenderableTexture::begin(void* cmdList) {
	auto* dxCmdList = static_cast<ID3D12GraphicsCommandList4*>(cmdList);

	//transitionStateTo(dxCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

	const D3D12_CPU_DESCRIPTOR_HANDLE* dsvCdh = (m_hasDepthTextures) ? &m_dsvHeapCDHs[context->getFrameIndex()] : &context->getDepthStencilViewCDH();
	dxCmdList->OMSetRenderTargets(1, &m_rtvHeapCDHs[context->getFrameIndex()], true, dsvCdh);

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
	dxCmdList->ClearRenderTargetView(m_rtvHeapCDHs[context->getFrameIndex()], clearColor, 0, nullptr);
	if (m_hasDepthTextures) {
		dxCmdList->ClearDepthStencilView(m_dsvHeapCDHs[context->getFrameIndex()], D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}
}

void DX12RenderableTexture::resize(int width, int height) {
	if (width == m_width && height == m_height) {
		return;
	}

	m_width = width;
	m_height = height;
	createTextures();
}

ID3D12Resource1* DX12RenderableTexture::getResource() const {
	return textureDefaultBuffers[context->getFrameIndex()].Get();
}

void DX12RenderableTexture::createTextures() {

	for (unsigned int i = 0; i < context->getNumSwapBuffers(); i++) {
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.Width = m_width;
		textureDesc.Height = m_height;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.MipLevels = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		D3D12_CLEAR_VALUE clearValue = { textureDesc.Format, { 0.1f, 0.2f, 0.3f, 1.0f } };

		state[i] = D3D12_RESOURCE_STATE_RENDER_TARGET;
		// A texture rarely updates its data, if at all, so it is stored in a default heap
		ThrowIfFailed(context->getDevice()->CreateCommittedResource(&DX12Utils::sDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &textureDesc, state[i], &clearValue, IID_PPV_ARGS(&textureDefaultBuffers[i])));
		textureDefaultBuffers[i]->SetName(L"Renderable texture default buffer");

		// Create a shader resource view (descriptor that points to the texture and describes it)
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		context->getDevice()->CreateShaderResourceView(textureDefaultBuffers[i].Get(), &srvDesc, srvHeapCDHs[i]);

		// Create a unordered access view
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = textureDesc.Format;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		context->getDevice()->CreateUnorderedAccessView(textureDefaultBuffers[i].Get(), nullptr, &uavDesc, uavHeapCDHs[i]);

		// Create a render target view
		context->getDevice()->CreateRenderTargetView(textureDefaultBuffers[i].Get(), nullptr, m_rtvHeapCDHs[i]);
	}

	if (m_hasDepthTextures) {
		createDepthTextures();
	}
}

void DX12RenderableTexture::createDepthTextures() {
	for (unsigned int i = 0; i < context->getNumSwapBuffers(); i++) {

		D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
		depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
		depthOptimizedClearValue.DepthStencil.Stencil = 0;

		D3D12_RESOURCE_DESC bufferDesc{};
		bufferDesc.Format = DXGI_FORMAT_D32_FLOAT;
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
		depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;
		context->getDevice()->CreateDepthStencilView(m_depthStencilBuffers[i].Get(), &depthStencilDesc, m_dsvHeapCDHs[i]);
	}
}
