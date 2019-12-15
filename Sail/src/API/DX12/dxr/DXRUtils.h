#pragma once
#include "../DX12API.h"
#include "../shader/DXILShaderCompiler.h"

namespace DXRUtils {
	class PSOBuilder {
	public:
		PSOBuilder();
		~PSOBuilder();

		D3D12_STATE_SUBOBJECT* append(D3D12_STATE_SUBOBJECT_TYPE type, const void* desc);
		void addLibrary(const std::string& shaderPath, const std::vector<LPCWSTR>& names, const std::vector<DxcDefine>& defines = std::vector<DxcDefine>());
		void addHitGroup(LPCWSTR exportName, LPCWSTR closestHitShaderImport, LPCWSTR anyHitShaderImport = nullptr, LPCWSTR intersectionShaderImport = nullptr, D3D12_HIT_GROUP_TYPE type = D3D12_HIT_GROUP_TYPE_TRIANGLES);
		void addSignatureToShaders(const std::vector<LPCWSTR>& shaderNames, ID3D12RootSignature** rootSignature);
		void setGlobalSignature(ID3D12RootSignature** rootSignature);
		void setMaxPayloadSize(UINT size);
		void setMaxAttributeSize(UINT size);
		void setMaxRecursionDepth(UINT depth);

		ID3D12StateObject* build(ID3D12Device5* device);
	private:

		DXILShaderCompiler m_dxilCompiler;

		D3D12_STATE_SUBOBJECT m_start[100];
		UINT m_numSubobjects;

		// Settings
		UINT m_maxPayloadSize;
		UINT m_maxAttributeSize;
		UINT m_maxRecursionDepth;
		ID3D12RootSignature** m_globalRootSignature;

		// Objects to keep in memory until generate() is called
		std::vector<LPCWSTR> m_shaderNames;
		std::vector<std::vector<D3D12_EXPORT_DESC>> m_exportDescs;
		std::vector<D3D12_DXIL_LIBRARY_DESC> m_libraryDescs;
		std::vector<std::vector<LPCWSTR>> m_associationNames;
		std::vector<D3D12_HIT_GROUP_DESC> m_hitGroupDescs;
		std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> m_exportAssociations;
	};


	struct ShaderTableData {
		UINT64 SizeInBytes;
		UINT32 StrideInBytes;
		wComPtr<ID3D12Resource> Resource = nullptr;
		void release() {
			if (Resource) {
				Resource->Release();
				Resource = nullptr;
			}
		}
	};

	class ShaderTableBuilder {
	public:

		ShaderTableBuilder(UINT numInstances, ID3D12StateObject* pso, UINT maxBytesPerInstance = 32);
		~ShaderTableBuilder();

		void addShader(const LPCWSTR& shaderName);
		void addDescriptor(UINT64& descriptor, UINT instance = 0);
		void addConstants(UINT numConstants, float* constants, UINT instance = 0);

		ShaderTableData build(ID3D12Device5* device);

	private:
		wComPtr<ID3D12StateObjectProperties> m_soProps;
		std::vector<LPCWSTR> m_shaderNames;
		UINT m_numInstances;
		UINT m_maxBytesPerInstance;

		void** m_data;
		UINT* m_dataOffsets;
	};
}