#pragma once

#include "Sail/api/Texture.h"
#include "DX12ATexture.h"
#include "Sail/resources/TextureData.h"
#include "../DX12API.h"
#include "DescriptorHeap.h"
#include <mutex>

class DX12DDSTexture : public Texture, public virtual DX12ATexture {
public:
	DX12DDSTexture(const std::string& filename);
	~DX12DDSTexture();

	// initBuffers is called once during its first bind
	// It is used to create resource objects that needs an open command list
	void initBuffers(ID3D12GraphicsCommandList4* cmdList);
	bool hasBeenInitialized() const;
	ID3D12Resource* getResource() const;

private:
	void generateMips(ID3D12GraphicsCommandList4* cmdList);

private:
	static const unsigned int MIP_LEVELS = 4;

	DX12API* context;
	//TextureData& m_textureData;
	D3D12_RESOURCE_DESC m_textureDesc;
	wComPtr<ID3D12Resource> m_textureUploadBuffer;

	std::mutex m_initializeMutex;
	bool m_isInitialized;

	std::vector<D3D12_SUBRESOURCE_DATA> m_subresources;

	std::unique_ptr<uint8_t[]> m_ddsData;

};