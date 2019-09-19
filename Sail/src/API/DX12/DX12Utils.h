#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <functional>
#include <map>

namespace DX12Utils {

	const D3D12_HEAP_PROPERTIES sUploadHeapProperties = {
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0,
	};

	const D3D12_HEAP_PROPERTIES sDefaultHeapProps = {
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0
	};

	void UpdateDefaultBufferData(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const void* data, UINT64 byteSize, UINT64 offset, ID3D12Resource1* defaultBuffer, ID3D12Resource1** uploadBuffer);
	ID3D12Resource1* CreateBuffer(ID3D12Device5* device, UINT64 size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps, D3D12_RESOURCE_DESC* bufDesc = nullptr);
	void SetResourceTransitionBarrier(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter);




	// Following inline methods are taken from d3dx12.h authored by microsoft
	// Some are slightly modified

	//------------------------------------------------------------------------------------------------
	// Row-by-row memcpy
	inline void MemcpySubresource(
		_In_ const D3D12_MEMCPY_DEST* pDest,
		_In_ const D3D12_SUBRESOURCE_DATA* pSrc,
		SIZE_T RowSizeInBytes,
		UINT NumRows,
		UINT NumSlices) {
		for (UINT z = 0; z < NumSlices; ++z) {
			BYTE* pDestSlice = reinterpret_cast<BYTE*>(pDest->pData) + pDest->SlicePitch * z;
			const BYTE* pSrcSlice = reinterpret_cast<const BYTE*>(pSrc->pData) + pSrc->SlicePitch * z;
			for (UINT y = 0; y < NumRows; ++y) {
				memcpy(pDestSlice + pDest->RowPitch * y,
					pSrcSlice + pSrc->RowPitch * y,
					RowSizeInBytes);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	// All arrays must be populated (e.g. by calling GetCopyableFootprints)
	inline UINT64 UpdateSubresources(
		_In_ ID3D12GraphicsCommandList* pCmdList,
		_In_ ID3D12Resource* pDestinationResource,
		_In_ ID3D12Resource* pIntermediate,
		_In_range_(0, D3D12_REQ_SUBRESOURCES) UINT FirstSubresource,
		_In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource) UINT NumSubresources,
		UINT64 RequiredSize,
		_In_reads_(NumSubresources) const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
		_In_reads_(NumSubresources) const UINT* pNumRows,
		_In_reads_(NumSubresources) const UINT64* pRowSizesInBytes,
		_In_reads_(NumSubresources) const D3D12_SUBRESOURCE_DATA* pSrcData) {
		// Minor validation
		auto IntermediateDesc = pIntermediate->GetDesc();
		auto DestinationDesc = pDestinationResource->GetDesc();
		if (IntermediateDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
			IntermediateDesc.Width < RequiredSize + pLayouts[0].Offset ||
			RequiredSize > SIZE_T(-1) ||
			(DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER &&
			(FirstSubresource != 0 || NumSubresources != 1))) {
			return 0;
		}

		BYTE* pData;
		HRESULT hr = pIntermediate->Map(0, nullptr, reinterpret_cast<void**>(&pData));
		if (FAILED(hr)) {
			return 0;
		}

		for (UINT i = 0; i < NumSubresources; ++i) {
			if (pRowSizesInBytes[i] > SIZE_T(-1)) return 0;
			D3D12_MEMCPY_DEST DestData = { pData + pLayouts[i].Offset, pLayouts[i].Footprint.RowPitch, SIZE_T(pLayouts[i].Footprint.RowPitch) * SIZE_T(pNumRows[i]) };
			MemcpySubresource(&DestData, &pSrcData[i], static_cast<SIZE_T>(pRowSizesInBytes[i]), pNumRows[i], pLayouts[i].Footprint.Depth);
		}
		pIntermediate->Unmap(0, nullptr);

		if (DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
			pCmdList->CopyBufferRegion(
				pDestinationResource, 0, pIntermediate, pLayouts[0].Offset, pLayouts[0].Footprint.Width);
		} else {
			for (UINT i = 0; i < NumSubresources; ++i) {
				D3D12_TEXTURE_COPY_LOCATION Dst{};
				Dst.pResource = pDestinationResource;
				Dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				Dst.SubresourceIndex = i + FirstSubresource;

				D3D12_TEXTURE_COPY_LOCATION Src{};
				Src.pResource = pIntermediate;
				Src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
				Src.PlacedFootprint = pLayouts[i];

				pCmdList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
			}
		}
		return RequiredSize;
	}

	//------------------------------------------------------------------------------------------------
	// Heap-allocating UpdateSubresources implementation
	inline UINT64 UpdateSubresources(
		_In_ ID3D12GraphicsCommandList* pCmdList,
		_In_ ID3D12Resource* pDestinationResource,
		_In_ ID3D12Resource* pIntermediate,
		UINT64 IntermediateOffset,
		_In_range_(0, D3D12_REQ_SUBRESOURCES) UINT FirstSubresource,
		_In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource) UINT NumSubresources,
		_In_reads_(NumSubresources) D3D12_SUBRESOURCE_DATA* pSrcData) {
		UINT64 RequiredSize = 0;
		UINT64 MemToAlloc = static_cast<UINT64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * NumSubresources;
		if (MemToAlloc > SIZE_MAX) {
			return 0;
		}
		void* pMem = HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(MemToAlloc));
		if (pMem == nullptr) {
			return 0;
		}
		auto pLayouts = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(pMem);
		UINT64* pRowSizesInBytes = reinterpret_cast<UINT64*>(pLayouts + NumSubresources);
		UINT* pNumRows = reinterpret_cast<UINT*>(pRowSizesInBytes + NumSubresources);

		auto Desc = pDestinationResource->GetDesc();
		ID3D12Device* pDevice = nullptr;
		pDestinationResource->GetDevice(__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice));
		pDevice->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, IntermediateOffset, pLayouts, pNumRows, pRowSizesInBytes, &RequiredSize);
		pDevice->Release();

		UINT64 Result = UpdateSubresources(pCmdList, pDestinationResource, pIntermediate, FirstSubresource, NumSubresources, RequiredSize, pLayouts, pNumRows, pRowSizesInBytes, pSrcData);
		HeapFree(GetProcessHeap(), 0, pMem);
		return Result;
	}




	class RootSignature {
	private:
		static D3D12_STATIC_SAMPLER_DESC sDefaultSampler;
	public:
		RootSignature(const std::string& name);

		void add32BitConstants();
		void addDescriptorTable(const std::string& name, D3D12_DESCRIPTOR_RANGE_TYPE type, unsigned int shaderRegister, unsigned int space = 0, unsigned int numDescriptors = 1);
		void addCBV(const std::string& name, unsigned int shaderRegister, unsigned int space = 0);
		void addSRV(const std::string& name, unsigned int shaderRegister, unsigned int space = 0);
		void addUAV(const std::string& name, unsigned int shaderRegister, unsigned int space = 0);
		void addStaticSampler(const D3D12_STATIC_SAMPLER_DESC& desc = sDefaultSampler);

		void build(ID3D12Device5* device, const D3D12_ROOT_SIGNATURE_FLAGS& flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);
		ID3D12RootSignature** get();
		unsigned int getIndex(const std::string& name);

		// Allows an unordered specification to run in root parameter order
		void doInOrder(std::function<void(const std::string&)> func);

	private:
		std::string m_name;
		std::vector<D3D12_ROOT_PARAMETER> m_rootParams;
		std::vector<D3D12_STATIC_SAMPLER_DESC> m_staticSamplerDescs;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_signature;
		std::vector<std::string> m_order;
	};

}