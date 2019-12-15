#include "pch.h"
#include "DXRUtils.h"
#include "../DX12Utils.h"

DXRUtils::PSOBuilder::PSOBuilder()
	: m_numSubobjects(0)
	, m_maxAttributeSize(sizeof(float) * 2)
	, m_maxPayloadSize(0)
	, m_maxRecursionDepth(1)
	, m_globalRootSignature(nullptr) {

	m_dxilCompiler.init();

	//WARNING! If any of these vectors expand while adding elements to them, all references will brake crashing the program when buiding the PSO.
	//TODO: Fix it
	m_associationNames.reserve(10);
	m_exportAssociations.reserve(10);
	m_shaderNames.reserve(10);
	m_exportDescs.reserve(10);
	m_libraryDescs.reserve(10);
	m_hitGroupDescs.reserve(10);
}

DXRUtils::PSOBuilder::~PSOBuilder() {}

D3D12_STATE_SUBOBJECT* DXRUtils::PSOBuilder::append(D3D12_STATE_SUBOBJECT_TYPE type, const void* desc) {
	D3D12_STATE_SUBOBJECT* so = m_start + m_numSubobjects++;
	so->Type = type;
	so->pDesc = desc;
	return so;
}

void DXRUtils::PSOBuilder::addLibrary(const std::string& shaderPath, const std::vector<LPCWSTR>& names, const std::vector<DxcDefine>& defines) {

	// Add names to the list of names/export to be configured in generate()
	m_shaderNames.insert(m_shaderNames.end(), names.begin(), names.end());

	DXILShaderCompiler::Desc shaderDesc;
	shaderDesc.compileArguments.push_back(L"/Gis"); // Declare all variables and values as precise
#ifdef _DEBUG
	shaderDesc.compileArguments.push_back(L"/Zi"); // Debug info
#endif
	std::wstring stemp = std::wstring(shaderPath.begin(), shaderPath.end());
	shaderDesc.filePath = stemp.c_str();
	shaderDesc.entryPoint = L"";
	shaderDesc.targetProfile = L"lib_6_3";
	shaderDesc.defines = defines;

	IDxcBlob* pShaders = nullptr;
	ThrowIfFailed(m_dxilCompiler.compile(&shaderDesc, &pShaders));

	m_exportDescs.emplace_back(names.size());
	std::vector<D3D12_EXPORT_DESC>& dxilExports = m_exportDescs.back();
	for (int i = 0; i < names.size(); i++) {
		auto& desc = dxilExports[i];
		desc.Name = names[i];
		desc.ExportToRename = nullptr;
		desc.Flags = D3D12_EXPORT_FLAG_NONE;
	}

	// Set up DXIL description
	m_libraryDescs.emplace_back();
	D3D12_DXIL_LIBRARY_DESC& dxilLibraryDesc = m_libraryDescs.back();;
	dxilLibraryDesc.DXILLibrary.pShaderBytecode = pShaders->GetBufferPointer();
	dxilLibraryDesc.DXILLibrary.BytecodeLength = pShaders->GetBufferSize();
	dxilLibraryDesc.pExports = &dxilExports[0];
	dxilLibraryDesc.NumExports = UINT(dxilExports.size());

	append(D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &dxilLibraryDesc);
}

void DXRUtils::PSOBuilder::addHitGroup(LPCWSTR exportName, LPCWSTR closestHitShaderImport, LPCWSTR anyHitShaderImport, LPCWSTR intersectionShaderImport, D3D12_HIT_GROUP_TYPE type) {
	//Init hit group
	m_hitGroupDescs.emplace_back();
	D3D12_HIT_GROUP_DESC& hitGroupDesc = m_hitGroupDescs.back();
	hitGroupDesc.AnyHitShaderImport = anyHitShaderImport;
	hitGroupDesc.ClosestHitShaderImport = closestHitShaderImport;
	hitGroupDesc.HitGroupExport = exportName;
	hitGroupDesc.IntersectionShaderImport = intersectionShaderImport;
	hitGroupDesc.Type = type;

	append(D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &hitGroupDesc);
}

void DXRUtils::PSOBuilder::addSignatureToShaders(const std::vector<LPCWSTR>& shaderNames, ID3D12RootSignature** rootSignature) {
	//TODO: pass in different rootSignatures to different shaders
	D3D12_STATE_SUBOBJECT* signatureSO = append(D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, rootSignature);

	m_associationNames.emplace_back(shaderNames);

	// Bind local root signature shaders
	m_exportAssociations.emplace_back();
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION& rayGenLocalRootAssociation = m_exportAssociations.back();
	rayGenLocalRootAssociation.pExports = &m_associationNames.back()[0];
	rayGenLocalRootAssociation.NumExports = UINT(shaderNames.size());
	rayGenLocalRootAssociation.pSubobjectToAssociate = signatureSO; //<-- address to local root subobject

	append(D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &rayGenLocalRootAssociation);
}

void DXRUtils::PSOBuilder::setGlobalSignature(ID3D12RootSignature** rootSignature) {
	m_globalRootSignature = rootSignature;
}

void DXRUtils::PSOBuilder::setMaxPayloadSize(UINT size) {
	m_maxPayloadSize = size;
}

void DXRUtils::PSOBuilder::setMaxAttributeSize(UINT size) {
	m_maxAttributeSize = size;
}

void DXRUtils::PSOBuilder::setMaxRecursionDepth(UINT depth) {
	m_maxRecursionDepth = depth;
}

ID3D12StateObject* DXRUtils::PSOBuilder::build(ID3D12Device5* device) {
	// Init shader config
	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
	shaderConfig.MaxAttributeSizeInBytes = m_maxAttributeSize;
	shaderConfig.MaxPayloadSizeInBytes = m_maxPayloadSize;
	auto configSO = append(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &shaderConfig);

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderConfigAssociation;
	if (!m_shaderNames.empty()) {
		// Bind the payload size to the programs
		shaderConfigAssociation.pExports = &m_shaderNames[0];
		shaderConfigAssociation.NumExports = UINT(m_shaderNames.size());
		shaderConfigAssociation.pSubobjectToAssociate = configSO; //<-- address to shader config subobject
		append(D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &shaderConfigAssociation);
	}
	// Init pipeline config
	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig;
	pipelineConfig.MaxTraceRecursionDepth = m_maxRecursionDepth;
	append(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &pipelineConfig);

	// Append the global root signature (I am GROOT)
	if (m_globalRootSignature)
		append(D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, m_globalRootSignature);

	// Create the state
	D3D12_STATE_OBJECT_DESC desc;
	desc.NumSubobjects = m_numSubobjects;
	desc.pSubobjects = m_start;
	desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

	ID3D12StateObject* pso;
	HRESULT hr = device->CreateStateObject(&desc, IID_PPV_ARGS(&pso));
	if (!SUCCEEDED(hr)) {
		_com_error err2(hr);
		std::cout << "Device Status: " << err2.ErrorMessage() << std::endl;

		hr = device->GetDeviceRemovedReason();
		_com_error err(hr);
		OutputDebugString(err.ErrorMessage());
		OutputDebugString(L"\n");
		std::cout << err.ErrorMessage() << std::endl;
	}
	ThrowIfFailed(hr);

	return pso;
}

DXRUtils::ShaderTableBuilder::ShaderTableBuilder(UINT numInstances, ID3D12StateObject* pso, UINT maxBytesPerInstance)
	: m_soProps(nullptr)
	, m_numInstances(numInstances)
	, m_maxBytesPerInstance(maxBytesPerInstance) {
	// Get the properties of the pre-built pipeline state object
	ThrowIfFailed(pso->QueryInterface(IID_PPV_ARGS(&m_soProps)));

	m_data = SAIL_NEW void* [m_numInstances];
	m_dataOffsets = SAIL_NEW UINT[m_numInstances];
	for (UINT i = 0; i < m_numInstances; i++) {
		m_data[i] = malloc(maxBytesPerInstance);
		m_dataOffsets[i] = 0;
	}
}

DXRUtils::ShaderTableBuilder::~ShaderTableBuilder() {
	for (UINT i = 0; i < m_numInstances; i++)
		free(m_data[i]);

	delete[] m_data;
	delete[] m_dataOffsets;
}

DXRUtils::ShaderTableData DXRUtils::ShaderTableBuilder::build(ID3D12Device5* device) {
	ShaderTableData shaderTable;

	UINT sizeOfLargestInstance = 0;
	for (UINT i = 0; i < m_numInstances; i++) {
		if (m_dataOffsets[i] > sizeOfLargestInstance) {
			sizeOfLargestInstance = m_dataOffsets[i];
		}
	}
	UINT size = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + sizeOfLargestInstance;

	UINT alignTo = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	UINT padding = (alignTo - (size % alignTo)) % alignTo;

	UINT alignedSize = size + padding;

	shaderTable.StrideInBytes = alignedSize;
	shaderTable.SizeInBytes = shaderTable.StrideInBytes * m_numInstances;
	shaderTable.Resource = DX12Utils::CreateBuffer(device, std::max((UINT64)1, shaderTable.SizeInBytes), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, DX12Utils::sUploadHeapProperties);
	shaderTable.Resource->SetName(L"SHADER_TABLE");

	// Map the buffer
	// Use a char* to to pointer arithmetic per byte
	char* pData;
	shaderTable.Resource->Map(0, nullptr, (void**)&pData);
	{
		assert(m_shaderNames.size() == m_numInstances && "All instances do not have a shader name associated with it!");
		for (unsigned int i = 0; i < m_numInstances; i++) {
			auto& shader = m_shaderNames[i];

			// Copy shader identifier
			void* shaderID = nullptr;
			if (shader == L"NULL") {
				// NULL shader identifier is valid and will cause no shader to be executed
				pData += alignedSize; // No data, just append padding
			} else {
				shaderID = m_soProps->GetShaderIdentifier(shader);
				assert(shaderID != nullptr && "Shader Identifier not found in stateObject");
				memcpy(pData, shaderID, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
				pData += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
				// Copy other data (descriptors, constants)
				memcpy(pData, m_data[i], m_dataOffsets[i]);
				pData += alignedSize - D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES; // Append padding
			}
		}
	}
	shaderTable.Resource->Unmap(0, nullptr);

	return shaderTable;
}

void DXRUtils::ShaderTableBuilder::addShader(const LPCWSTR& shaderName) {
	m_shaderNames.emplace_back(shaderName);
}

void DXRUtils::ShaderTableBuilder::addDescriptor(UINT64& descriptor, UINT instance) {
	assert(instance < m_numInstances && "DXRUtils::ShaderTableBuilder::addDescriptor");
	auto ptr = static_cast<char*>(m_data[instance]) + m_dataOffsets[instance];
	*(UINT64*)ptr = descriptor;
	m_dataOffsets[instance] += sizeof(descriptor);
	assert(m_dataOffsets[instance] <= m_maxBytesPerInstance && "DXRUtils::ShaderTableBuilder::addDescriptor bytesPerInstance is too small!");
}

void DXRUtils::ShaderTableBuilder::addConstants(UINT numConstants, float* constants, UINT instance) {
	assert(instance < m_numInstances && "DXRUtils::ShaderTableBuilder::addConstants");
	auto ptr = static_cast<char*>(m_data[instance]) + m_dataOffsets[instance];
	memcpy(ptr, constants, sizeof(float) * numConstants);
	m_dataOffsets[instance] += sizeof(float) * numConstants;
	assert(m_dataOffsets[instance] <= m_maxBytesPerInstance && "DXRUtils::ShaderTableBuilder::addConstants");
}
