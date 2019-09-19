#include "pch.h"
#include "DX12Texture.h"
#include "Sail/Application.h"
#include "../DX12Utils.h"

Texture* Texture::Create(const std::string& filename) {
	return new DX12Texture(filename);
}

DX12Texture::DX12Texture(const std::string& filename) 
	: m_isInitialized(false)
	, m_textureData(getTextureData(filename))
{
	context = Application::getInstance()->getAPI<DX12API>();
	// Dont create one resource per swap buffer
	useOneResource = true;

	m_textureDesc = {};
	m_textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // TODO: read this from texture data
	m_textureDesc.Width = m_textureData.getWidth();
	m_textureDesc.Height = m_textureData.getHeight();
	m_textureDesc.DepthOrArraySize = 1;
	m_textureDesc.MipLevels = 1;
	m_textureDesc.SampleDesc.Count = 1;
	m_textureDesc.SampleDesc.Quality = 0;
	m_textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	m_textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	m_textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	// A texture rarely updates its data, if at all, so it is stored in a default heap
	state[0] = D3D12_RESOURCE_STATE_COPY_DEST;
	ThrowIfFailed(context->getDevice()->CreateCommittedResource(&DX12Utils::sDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &m_textureDesc, state[0], nullptr, IID_PPV_ARGS(&textureDefaultBuffers[0])));
	textureDefaultBuffers[0]->SetName((std::wstring(L"Texture default buffer for ") + std::wstring(filename.begin(), filename.end())).c_str());

	// Create a shader resource view (descriptor that points to the texture and describes it)
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_textureDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	context->getDevice()->CreateShaderResourceView(textureDefaultBuffers[0].Get(), &srvDesc, srvHeapCDHs[0]);

	// Dont allow UAV access
	uavHeapCDHs[0] = {0};

}

DX12Texture::~DX12Texture() {

}

void DX12Texture::initBuffers(ID3D12GraphicsCommandList4* cmdList) {
	//The lock_guard will make sure multiple threads wont try to initialize the same texture
	std::lock_guard<std::mutex> lock(m_initializeMutex);
	if (m_isInitialized)
		return;

	UINT64 textureUploadBufferSize;
	// this function gets the size an upload buffer needs to be to upload a texture to the gpu.
	// each row must be 256 byte aligned except for the last row, which can just be the size in bytes of the row
	// eg. textureUploadBufferSize = ((((width * numBytesPerPixel) + 255) & ~255) * (height - 1)) + (width * numBytesPerPixel);
	context->getDevice()->GetCopyableFootprints(&m_textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

	// Create the upload heap
	// TODO: release the upload heap when it has been copied to the default buffer
	// This could be done in a buffer manager owned by dx12api
	m_textureUploadBuffer.Attach(DX12Utils::CreateBuffer(context->getDevice(), textureUploadBufferSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, DX12Utils::sUploadHeapProperties));
	m_textureUploadBuffer->SetName(L"Texture upload buffer");

	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = m_textureData.getTextureData();
	textureData.RowPitch = m_textureData.getWidth() * m_textureData.getBytesPerPixel();
	textureData.SlicePitch = textureData.RowPitch * m_textureData.getHeight();
	// Copy the upload buffer contents to the default heap using a helper method from d3dx12.h
	DX12Utils::UpdateSubresources(cmdList, textureDefaultBuffers[0].Get(), m_textureUploadBuffer.Get(), 0, 0, 1, &textureData);
	//DX12Utils::SetResourceTransitionBarrier(cmdList, textureDefaultBuffer.Get(), state, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	//transitionStateTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	m_isInitialized = true;
}

bool DX12Texture::hasBeenInitialized() const {
	return m_isInitialized;
}

ID3D12Resource1* DX12Texture::getResource() const {
	return textureDefaultBuffers[0].Get();
}
