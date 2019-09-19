#include "pch.h"
#include "DXRBase.h"
#include "Sail/Application.h"
#include "Sail/api/shader/ShaderPipeline.h"
#include "API/DX12/DX12VertexBuffer.h"
#include "API/DX12/DX12IndexBuffer.h"
#include "API/DX12/resources/DX12Texture.h"

DXRBase::DXRBase(const std::string& shaderFilename)
: m_shaderFilename(shaderFilename) 
{
	m_context = Application::getInstance()->getAPI<DX12API>();

	// Create frame resources (one per swap buffer)
	// Only one TLAS is used for the whole scene
	m_DXR_TopBuffer = m_context->createFrameResource<AccelerationStructureBuffers>();
	m_DXR_BottomBuffers = m_context->createFrameResource<std::vector<AccelerationStructureBuffers>>();
	m_rayGenShaderTable = m_context->createFrameResource<DXRUtils::ShaderTableData>();
	m_missShaderTable = m_context->createFrameResource<DXRUtils::ShaderTableData>();
	m_hitGroupShaderTable = m_context->createFrameResource<DXRUtils::ShaderTableData>();

	// Create root signatures
	createDXRGlobalRootSignature();
	createRayGenLocalRootSignature();
	createHitGroupLocalRootSignature();
	createMissLocalRootSignature();

	createRaytracingPSO();
	createShaderResources();

}

DXRBase::~DXRBase() {
	m_rtPipelineState->Release();
	for (auto& blasList : m_DXR_BottomBuffers) {
		for (auto& blas : blasList) {
			blas.release();
		}
	}
	for (auto& tlas : m_DXR_TopBuffer) {
		tlas.release();
	}
	for (auto& st : m_rayGenShaderTable) {
		st.release();
	}
	for (auto& st : m_missShaderTable) {
		st.release();
	}
	for (auto& st : m_hitGroupShaderTable) {
		st.release();
	}
		
}

void DXRBase::updateAccelerationStructures(const std::vector<Renderer::RenderCommand>& sceneGeometry, ID3D12GraphicsCommandList4* cmdList) {

	unsigned int frameIndex = m_context->getFrameIndex();

	// Update descriptors for vertices, indices, textures etc
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_rtHeapCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_rtHeapGPUHandle;
	m_rtMeshHandles.clear();
	unsigned int i = 0;
	for (auto& geometry : sceneGeometry) {
		auto& mesh = geometry.mesh;

		MeshHandles handles;
		handles.vertexBufferHandle = static_cast<const DX12VertexBuffer&>(mesh->getVertexBuffer()).getBuffer()->GetGPUVirtualAddress();
		if (mesh->getNumIndices() > 0)
			handles.indexBufferHandle = static_cast<const DX12IndexBuffer&>(mesh->getIndexBuffer()).getBuffer()->GetGPUVirtualAddress();
		
		// Three textures
		for (unsigned int textureNum = 0; textureNum < 3; textureNum++) {
			DX12Texture* texture = static_cast<DX12Texture*>(mesh->getMaterial()->getTexture(textureNum));
			bool hasTexture = (textureNum == 0) ? mesh->getMaterial()->getPhongSettings().hasDiffuseTexture : mesh->getMaterial()->getPhongSettings().hasNormalTexture;
			hasTexture = (textureNum == 2) ? mesh->getMaterial()->getPhongSettings().hasSpecularTexture : hasTexture;
			if (hasTexture) {
				// Make sure textures have initialized / uploaded their data to its default buffer
				if (!texture->hasBeenInitialized()) {
					texture->initBuffers(cmdList);
				}

				// Copy SRV to DXR heap
				m_context->getDevice()->CopyDescriptorsSimple(1, cpuHandle, texture->getSrvCDH(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				handles.textureHandles[textureNum] = gpuHandle;
			}
			// Increase pointer regardless of if the texture existed or not to keep to order in the SBT
			cpuHandle.ptr += m_heapIncr;
			gpuHandle.ptr += m_heapIncr;
		}

		// Update per mesh data
		// Such as flags telling the shader to use indices, textures or not
		unsigned int meshDataSize = sizeof(DXRShaderCommon::MeshData);
		DXRShaderCommon::MeshData meshData;
		meshData.flags = (mesh->getNumIndices() == 0) ? DXRShaderCommon::MESH_NO_FLAGS : DXRShaderCommon::MESH_USE_INDICES;
		meshData.flags |= (mesh->getMaterial()->getPhongSettings().hasDiffuseTexture) ?		DXRShaderCommon::MESH_HAS_DIFFUSE_TEX	: meshData.flags;
		meshData.flags |= (mesh->getMaterial()->getPhongSettings().hasNormalTexture) ?		DXRShaderCommon::MESH_HAS_NORMAL_TEX	: meshData.flags;
		meshData.flags |= (mesh->getMaterial()->getPhongSettings().hasSpecularTexture) ?	DXRShaderCommon::MESH_HAS_SPECULAR_TEX	: meshData.flags;
		meshData.color = mesh->getMaterial()->getPhongSettings().modelColor;
		m_meshCB[frameIndex]->updateData(&meshData, meshDataSize, i * meshDataSize);

		m_rtMeshHandles.emplace_back(handles);

		i++;
	}


	// TODO: run this on an async compute queue
	createBLAS(sceneGeometry, cmdList);
	createTLAS(sceneGeometry, cmdList);
	createShaderTables(sceneGeometry);

}

void DXRBase::updateCamera(Camera& cam) {
	DXRShaderCommon::SceneCBuffer newData = {};
	newData.cameraPosition = cam.getPosition();
	newData.projectionToWorld = glm::inverse(cam.getViewProjection());
	m_cameraCB[m_context->getFrameIndex()]->updateData(&newData, sizeof(newData));
}

void DXRBase::dispatch(DX12RenderableTexture* outputTexture, ID3D12GraphicsCommandList4* cmdList) {
	
	unsigned int frameIndex = m_context->getFrameIndex();

	outputTexture->transitionStateTo(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
	// Copy output texture srv to beginning of heap
	m_context->getDevice()->CopyDescriptorsSimple(1, m_rtDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), outputTexture->getUavCDH(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//Set constant buffer descriptor heap
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_rtDescriptorHeap.Get() };
	cmdList->SetDescriptorHeaps(ARRAYSIZE(descriptorHeaps), descriptorHeaps);


	// Let's raytrace
	outputTexture->transitionStateTo(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	//DX12Utils::SetResourceTransitionBarrier(cmdList, m_rtOutputUAV.resource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	D3D12_DISPATCH_RAYS_DESC raytraceDesc = {};
	raytraceDesc.Width = Application::getInstance()->getWindow()->getWindowWidth();
	raytraceDesc.Height = Application::getInstance()->getWindow()->getWindowHeight();
	raytraceDesc.Depth = 1;

	//set shader tables
	raytraceDesc.RayGenerationShaderRecord.StartAddress = m_rayGenShaderTable[frameIndex].Resource->GetGPUVirtualAddress();
	raytraceDesc.RayGenerationShaderRecord.SizeInBytes = m_rayGenShaderTable[frameIndex].SizeInBytes;

	raytraceDesc.MissShaderTable.StartAddress = m_missShaderTable[frameIndex].Resource->GetGPUVirtualAddress();
	raytraceDesc.MissShaderTable.StrideInBytes = m_missShaderTable[frameIndex].StrideInBytes;
	raytraceDesc.MissShaderTable.SizeInBytes = m_missShaderTable[frameIndex].SizeInBytes;

	raytraceDesc.HitGroupTable.StartAddress = m_hitGroupShaderTable[frameIndex].Resource->GetGPUVirtualAddress();
	raytraceDesc.HitGroupTable.StrideInBytes = m_hitGroupShaderTable[frameIndex].StrideInBytes;
	raytraceDesc.HitGroupTable.SizeInBytes = m_hitGroupShaderTable[frameIndex].SizeInBytes;

	// Bind the global root signature
	cmdList->SetComputeRootSignature(*m_dxrGlobalRootSignature->get());

	// Set acceleration structure
	cmdList->SetComputeRootShaderResourceView(m_dxrGlobalRootSignature->getIndex("AccelerationStructure"), m_DXR_TopBuffer[frameIndex].result->GetGPUVirtualAddress());
	// Set scene constant buffer
	cmdList->SetComputeRootConstantBufferView(m_dxrGlobalRootSignature->getIndex("SceneCBuffer"), m_cameraCB[frameIndex]->getBuffer()->GetGPUVirtualAddress());

	// Dispatch
	cmdList->SetPipelineState1(m_rtPipelineState.Get());
	cmdList->DispatchRays(&raytraceDesc);
}

bool DXRBase::onEvent(Event& event) {
	auto onResize = [&](WindowResizeEvent& event) {
		// Window changed size, resize output UAV
		createShaderResources(true);
		return true;
	};

	EventHandler::dispatch<WindowResizeEvent>(event, onResize);
	return true;
}

void DXRBase::createTLAS(const std::vector<Renderer::RenderCommand>& sceneGeometry, ID3D12GraphicsCommandList4* cmdList) {

	// Always rebuilds TLAS instead of updating it according to nvidia recommendations

	unsigned int numBLAS = sceneGeometry.size();
	unsigned int frameIndex = m_context->getFrameIndex();

	// First, get the size of the TLAS buffers and create them
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = numBLAS;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	m_DXR_TopBuffer[frameIndex].release();

	// Re-create the buffer
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	m_context->getDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	// Create the buffers
	if (m_DXR_TopBuffer[frameIndex].scratch == nullptr) {
		m_DXR_TopBuffer[frameIndex].scratch = DX12Utils::CreateBuffer(m_context->getDevice(), info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DX12Utils::sDefaultHeapProps);
		m_DXR_TopBuffer[frameIndex].scratch->SetName(L"TLAS_SCRATCH");

	}

	if (m_DXR_TopBuffer[frameIndex].result == nullptr) {
		m_DXR_TopBuffer[frameIndex].result = DX12Utils::CreateBuffer(m_context->getDevice(), info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, DX12Utils::sDefaultHeapProps);
		m_DXR_TopBuffer[frameIndex].result->SetName(L"TLAS_RESULT");
	}

	m_DXR_TopBuffer[frameIndex].instanceDesc = DX12Utils::CreateBuffer(m_context->getDevice(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * glm::max(numBLAS, 1U), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, DX12Utils::sUploadHeapProperties);
	m_DXR_TopBuffer[frameIndex].instanceDesc->SetName(L"TLAS_INSTANCE_DESC");

	D3D12_RAYTRACING_INSTANCE_DESC* pInstanceDesc;
	m_DXR_TopBuffer[frameIndex].instanceDesc->Map(0, nullptr, (void**)& pInstanceDesc);

	for (UINT i = 0; i < numBLAS; i++) {

		pInstanceDesc->InstanceID = i;                            // exposed to the shader via InstanceID()
		pInstanceDesc->InstanceContributionToHitGroupIndex = i;   // offset inside the shader-table. Unique for every instance since each geometry has different vertexbuffer/indexbuffer/textures
		pInstanceDesc->Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

		// apply transform from lambda function
		glm::mat3x4 m = sceneGeometry[i].transform;
		memcpy(pInstanceDesc->Transform, &m, sizeof(pInstanceDesc->Transform));

		pInstanceDesc->AccelerationStructure = m_DXR_BottomBuffers[frameIndex][i].result->GetGPUVirtualAddress();
		pInstanceDesc->InstanceMask = 0xFF;

		pInstanceDesc++;
	}
	// Unmap
	m_DXR_TopBuffer[frameIndex].instanceDesc->Unmap(0, nullptr);

	// Create the TLAS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.Inputs.InstanceDescs = m_DXR_TopBuffer[frameIndex].instanceDesc->GetGPUVirtualAddress();
	asDesc.DestAccelerationStructureData = m_DXR_TopBuffer[frameIndex].result->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = m_DXR_TopBuffer[frameIndex].scratch->GetGPUVirtualAddress();

	cmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	// UAV barrier needed before using the acceleration structures in a raytracing operation
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = m_DXR_TopBuffer[frameIndex].result.Get();
	cmdList->ResourceBarrier(1, &uavBarrier);
}

void DXRBase::createBLAS(const std::vector<Renderer::RenderCommand>& sceneGeometry, ID3D12GraphicsCommandList4* cmdList) {

	unsigned int numBLAS = sceneGeometry.size();
	unsigned int frameIndex = m_context->getFrameIndex();
	bool onlyUpdate = false;

	if (!onlyUpdate) {
		//std::cout << "FULL BLAS BUILD" << std::endl;
		// Release old buffers if they exist
		for (auto& blas : m_DXR_BottomBuffers[frameIndex]) {
			blas.release();
		}
		// Resize all BLAS vectors
		m_DXR_BottomBuffers[frameIndex].resize(numBLAS);
	}

	for (unsigned int i = 0; i < numBLAS; i++) {
		auto& geometry = sceneGeometry[i];
		auto& bottomBuffer = m_DXR_BottomBuffers[frameIndex][i];

		auto& vb = static_cast<const DX12VertexBuffer&>(geometry.mesh->getVertexBuffer());
		auto& ib = static_cast<const DX12IndexBuffer&>(geometry.mesh->getIndexBuffer());

		D3D12_RAYTRACING_GEOMETRY_DESC geomDesc[1] = {};
		geomDesc[0] = {};
		geomDesc[0].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		geomDesc[0].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geomDesc[0].Triangles.VertexBuffer.StartAddress = vb.getBuffer()->GetGPUVirtualAddress();
		geomDesc[0].Triangles.VertexBuffer.StrideInBytes = vb.getVertexDataStride();
		geomDesc[0].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		geomDesc[0].Triangles.VertexCount = geometry.mesh->getNumVertices();

		if (geometry.mesh->getNumIndices() > 0) {
			geomDesc[0].Triangles.IndexBuffer = ib.getBuffer()->GetGPUVirtualAddress();
			geomDesc[0].Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
			geomDesc[0].Triangles.IndexCount = UINT(geometry.mesh->getNumIndices());
		}

		// Get the size requirements for the scratch and AS buffers
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
		inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE; // Changing this flag depending on mesh can speed up performance significantly!
		/*if (onlyUpdate) {
			inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
		}*/
		inputs.NumDescs = 1;
		inputs.pGeometryDescs = geomDesc;
		inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
		m_context->getDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

		// TODO: make sure buffer size is >= info.UpdateScratchDataSize in bytes
		if (!onlyUpdate) {
			// Create the buffers. They need to support UAV, and since we are going to immediately use them, we create them with an unordered-access state
			bottomBuffer.scratch = DX12Utils::CreateBuffer(m_context->getDevice(), info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DX12Utils::sDefaultHeapProps);
			bottomBuffer.scratch->SetName(L"BLAS_SCRATCH");
			bottomBuffer.result = DX12Utils::CreateBuffer(m_context->getDevice(), info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, DX12Utils::sDefaultHeapProps);
			bottomBuffer.result->SetName(L"BLAS_RESULT");
		}

		// Create the bottom-level AS
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
		asDesc.Inputs = inputs;
		asDesc.ScratchAccelerationStructureData = bottomBuffer.scratch->GetGPUVirtualAddress();
		asDesc.DestAccelerationStructureData = bottomBuffer.result->GetGPUVirtualAddress();
		if (inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE)
			asDesc.SourceAccelerationStructureData = bottomBuffer.result->GetGPUVirtualAddress();

		cmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

		// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
		D3D12_RESOURCE_BARRIER uavBarrier = {};
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = m_DXR_BottomBuffers[frameIndex][i].result.Get();
		cmdList->ResourceBarrier(1, &uavBarrier);
	}

}

void DXRBase::createShaderResources(bool remake) {

	// Create some resources only once on init
	if (!m_rtDescriptorHeap || remake) {
		m_rtDescriptorHeap.Reset();
		D3D12_DESCRIPTOR_HEAP_DESC heapDescriptorDesc = {};
		heapDescriptorDesc.NumDescriptors = 2000; // TODO: this does not throw error when full
		heapDescriptorDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDescriptorDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		m_context->getDevice()->CreateDescriptorHeap(&heapDescriptorDesc, IID_PPV_ARGS(&m_rtDescriptorHeap));

		m_heapIncr = m_context->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// Create the UAV. Based on the root signature we created it should be the first entry
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_rtDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_rtDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

		// The first slot in the heap will be used for the output UAV, therefore we step once
		m_rtOutputTextureUavGPUHandle = m_rtDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

		cpuHandle.ptr += m_heapIncr;
		gpuHandle.ptr += m_heapIncr;

		// Store heap start for views that might update in runtime
		m_rtHeapCPUHandle = cpuHandle;
		m_rtHeapGPUHandle = gpuHandle;

		//// Ray gen settings CB
		//m_rayGenCBData.flags = RT_ENABLE_TA | RT_ENABLE_JITTER_AA;
		//m_rayGenCBData.numAORays = 5;
		//m_rayGenCBData.AORadius = 0.9f;
		//m_rayGenCBData.frameCount = 0;
		//m_rayGenCBData.GISamples = 1;
		//m_rayGenCBData.GIBounces = 1;
		//m_rayGenSettingsCB = std::make_unique<DX12ConstantBuffer>("Ray Gen Settings CB", sizeof(RayGenSettings), m_renderer);
		//m_rayGenSettingsCB->setData(&m_rayGenCBData, 0);

		// Scene CB
		{
			unsigned int size = sizeof(DXRShaderCommon::SceneCBuffer);
			void* initData = malloc(size);
			memset(initData, 0, size);
			for (unsigned int i = 0; i < m_context->getNumSwapBuffers(); i++) {
				m_cameraCB.emplace_back(std::make_unique<ShaderComponent::DX12ConstantBuffer>(initData, size, ShaderComponent::BIND_SHADER::CS, 0));
			}
			free(initData);
		}
		// Mesh CB
		{
			unsigned int size = sizeof(DXRShaderCommon::MeshCBuffer);
			void* initData = malloc(size);
			memset(initData, 0, size);
			for (unsigned int i = 0; i < m_context->getNumSwapBuffers(); i++) {
				m_meshCB.emplace_back(std::make_unique<ShaderComponent::DX12ConstantBuffer>(initData, size, ShaderComponent::BIND_SHADER::CS, 0));
			}
			free(initData);
		}
	}

}

void DXRBase::createShaderTables(const std::vector<Renderer::RenderCommand>& sceneGeometry) {

	// 	 "Shader tables can be modified freely by the application (with appropriate state barriers)"

	auto frameIndex = m_context->getFrameIndex();

	// Ray gen
	{
		if (m_rayGenShaderTable[frameIndex].Resource) {
			m_rayGenShaderTable[frameIndex].Resource->Release();
			m_rayGenShaderTable[frameIndex].Resource.Reset();
		}
		DXRUtils::ShaderTableBuilder tableBuilder(m_rayGenName, m_rtPipelineState.Get());
		tableBuilder.addDescriptor(m_rtOutputTextureUavGPUHandle.ptr);
		m_rayGenShaderTable[frameIndex] = tableBuilder.build(m_context->getDevice());
	}

	// Miss
	{
		if (m_missShaderTable[frameIndex].Resource) {
			m_missShaderTable[frameIndex].Resource->Release();
			m_missShaderTable[frameIndex].Resource.Reset();
		}
		DXRUtils::ShaderTableBuilder tableBuilder(m_missName, m_rtPipelineState.Get());
		//tableBuilder.addDescriptor(m_skyboxGPUDescHandle.ptr);
		m_missShaderTable[frameIndex] = tableBuilder.build(m_context->getDevice());
	}

	// Hit group
	// TODO: use different hit groups for regular shading, shadows, transparecy etc
	{
		if (m_hitGroupShaderTable[frameIndex].Resource) {
			m_hitGroupShaderTable[frameIndex].Resource->Release();
			m_hitGroupShaderTable[frameIndex].Resource.Reset();
		}
		DXRUtils::ShaderTableBuilder tableBuilder(m_hitGroupName, m_rtPipelineState.Get(), sceneGeometry.size(), 64U);
		for (unsigned int i = 0; i < sceneGeometry.size(); i++) {
			auto& mesh = sceneGeometry[i].mesh;

			m_localSignatureHitGroup->doInOrder([&](const std::string& parameterName) {
				if (parameterName == "VertexBuffer") {
					tableBuilder.addDescriptor(m_rtMeshHandles[i].vertexBufferHandle, i);
				} else if (parameterName == "IndexBuffer") {
					D3D12_GPU_VIRTUAL_ADDRESS nullAddr = 0;
					tableBuilder.addDescriptor((mesh->getNumIndices() > 0) ? m_rtMeshHandles[i].indexBufferHandle : nullAddr, i);
				} else if (parameterName == "MeshCBuffer") {
					D3D12_GPU_VIRTUAL_ADDRESS meshCBHandle = m_meshCB[frameIndex]->getBuffer()->GetGPUVirtualAddress();
					tableBuilder.addDescriptor(meshCBHandle, i);
				} else if (parameterName == "Textures") {
					// Three textures
					for (unsigned int textureNum = 0; textureNum < 3; textureNum++) {
						tableBuilder.addDescriptor(m_rtMeshHandles[i].textureHandles[textureNum].ptr, i);
					}
				} else {
					Logger::Error("Unhandled root signature parameter! ("+parameterName+")");
				}
					
			});
		}
		m_hitGroupShaderTable[frameIndex] = tableBuilder.build(m_context->getDevice());
	}
}

void DXRBase::createRaytracingPSO() {
	DXRUtils::PSOBuilder psoBuilder;
	psoBuilder.addLibrary(ShaderPipeline::DEFAULT_SHADER_LOCATION + "dxr/" + m_shaderFilename + ".hlsl", { m_rayGenName, m_closestHitName, m_missName });
	psoBuilder.addHitGroup(m_hitGroupName, m_closestHitName);
	psoBuilder.addSignatureToShaders({ m_rayGenName }, m_localSignatureRayGen->get());
	psoBuilder.addSignatureToShaders({ m_closestHitName }, m_localSignatureHitGroup->get());
	psoBuilder.addSignatureToShaders({ m_missName }, m_localSignatureMiss->get());
	psoBuilder.setMaxPayloadSize(sizeof(RayPayload));
	psoBuilder.setMaxRecursionDepth(MAX_RAY_RECURSION_DEPTH);
	psoBuilder.setGlobalSignature(m_dxrGlobalRootSignature->get());

	m_rtPipelineState = psoBuilder.build(m_context->getDevice());
}

void DXRBase::createDXRGlobalRootSignature() {
	m_dxrGlobalRootSignature = std::make_unique<DX12Utils::RootSignature>("dxrGlobal");
	m_dxrGlobalRootSignature->addSRV("AccelerationStructure", 0);
	m_dxrGlobalRootSignature->addCBV("SceneCBuffer", 0);

	m_dxrGlobalRootSignature->build(m_context->getDevice());
}

void DXRBase::createRayGenLocalRootSignature() {
	m_localSignatureRayGen = std::make_unique<DX12Utils::RootSignature>("RayGenLocal");
	m_localSignatureRayGen->addDescriptorTable("OutputUAV", D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0);

	m_localSignatureRayGen->build(m_context->getDevice(), D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
}

void DXRBase::createHitGroupLocalRootSignature() {
	m_localSignatureHitGroup = std::make_unique<DX12Utils::RootSignature>("HitGroupLocal");
	m_localSignatureHitGroup->addSRV("VertexBuffer", 1, 0);
	m_localSignatureHitGroup->addSRV("IndexBuffer", 1, 1);
	m_localSignatureHitGroup->addCBV("MeshCBuffer", 1, 0);
	m_localSignatureHitGroup->addDescriptorTable("Textures", D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 3); // Textures (t0, t1, t2)
	m_localSignatureHitGroup->addStaticSampler();

	m_localSignatureHitGroup->build(m_context->getDevice(), D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
}

void DXRBase::createMissLocalRootSignature() {
	m_localSignatureMiss = std::make_unique<DX12Utils::RootSignature>("MissLocal");
	//m_localSignatureMiss->addDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3); // Skybox
	m_localSignatureMiss->addStaticSampler();

	m_localSignatureMiss->build(m_context->getDevice(), D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
}
