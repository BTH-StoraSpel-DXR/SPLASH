#include "pch.h"
#include "DXRBase.h"
#include "Sail/Application.h"
#include "Sail/api/shader/ShaderPipeline.h"
#include "API/DX12/DX12VertexBuffer.h"
#include "API/DX12/DX12IndexBuffer.h"
#include "API/DX12/resources/DX12Texture.h"
#include "Sail/graphics/light/LightSetup.h"
#include "../renderer/DX12GBufferRenderer.h"
#include "Sail/entities/systems/Gameplay/LevelSystem/LevelSystem.h"
#include "../SPLASH/src/game/events/ResetWaterEvent.h"

#include "Sail/events/EventDispatcher.h"

DXRBase::DXRBase(const std::string& shaderFilename, DX12RenderableTexture** inputs)
	: m_shaderFilename(shaderFilename)
	, m_gbufferInputTextures(inputs)
	, m_brdfLUTPath("pbr/brdfLUT.tga")
	, m_waterChanged(false)
	, m_waterDataCPU(nullptr)
	, m_updateWater(nullptr)
	, m_mapSize(1.f)
	, m_mapStart(1.f)
{

	EventDispatcher::Instance().subscribe(Event::Type::WINDOW_RESIZE, this);
	EventDispatcher::Instance().subscribe(Event::Type::RESET_WATER, this);

	/*m_decalTexPaths[0] = "pbr/water/Water_001_COLOR.tga";
	m_decalTexPaths[1] = "pbr/water/Water_001_NORM.tga";
	m_decalTexPaths[2] = "pbr/water/Water_001_MAT.tga";*/
	m_decalTexPaths[0] = "pbr/splash/PuddleAlbedo.tga";
	m_decalTexPaths[1] = "pbr/splash/PuddleNM.tga";
	m_decalTexPaths[2] = "pbr/splash/puddleMRAo.tga";

	m_context = Application::getInstance()->getAPI<DX12API>();

	// Create frame resources (one per swap buffer)
	// Only one TLAS is used for the whole scene
	m_DXR_TopBuffer = m_context->createFrameResource<AccelerationStructureBuffers>();
	m_bottomBuffers = m_context->createFrameResource<std::unordered_map<Mesh*, InstanceList>>();
	m_bottomBuffers_Metaballs = m_context->createFrameResource<std::unordered_map<int, InstanceList>>();
	m_rayGenShaderTable = m_context->createFrameResource<DXRUtils::ShaderTableData>();
	m_missShaderTable = m_context->createFrameResource<DXRUtils::ShaderTableData>();
	m_hitGroupShaderTable = m_context->createFrameResource<DXRUtils::ShaderTableData>();
	m_gbufferStartGPUHandles = m_context->createFrameResource<D3D12_GPU_DESCRIPTOR_HANDLE>();

	// Create root signatures
	createDXRGlobalRootSignature();
	createRayGenLocalRootSignature();
	createHitGroupLocalRootSignature();
	createMissLocalRootSignature();
	createEmptyLocalRootSignature();

	createRaytracingPSO();
	createInitialShaderResources();

	m_decalsToRender = 0;
	unsigned int initData = 0;
	m_waterStructuredBuffer = std::make_unique<ShaderComponent::DX12StructuredBuffer>(&initData, 1, sizeof(unsigned int));
	m_waterArrSize = 1;
	m_waterArrSizes = glm::vec3(1.f, 1.f, 1.f);
}

DXRBase::~DXRBase() {
	Memory::SafeDeleteArr(m_waterDataCPU);
	Memory::SafeDeleteArr(m_updateWater);

	m_rtPipelineState->Release();
	for (auto& blasList : m_bottomBuffers) {
		for (auto& blas : blasList) {
			blas.second.blas.release();
		}
	}
	for (auto& blasList : m_bottomBuffers_Metaballs) {
		for (auto& blas : blasList) {
			blas.second.blas.release();
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

	for (auto& resource : m_metaballPositions_srv) {
		resource->Release();
	}

	for (auto& resource : m_aabb_desc_resources) {
		for (auto& resource2 : resource.second) {
			resource2->Release();
		}
	}

	EventDispatcher::Instance().unsubscribe(Event::Type::WINDOW_RESIZE, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::RESET_WATER, this);
}

void DXRBase::setGBufferInputs(DX12RenderableTexture** inputs) {
	m_gbufferInputTextures = inputs;
}

void DXRBase::updateAccelerationStructures(const std::vector<Renderer::RenderCommand>& sceneGeometry, ID3D12GraphicsCommandList4* cmdList, const std::vector<DXRBase::MetaballGroup*>& metaballGroups) {

	unsigned int frameIndex = m_context->getSwapIndex();
	unsigned int totalNumInstances = 0;

	auto flagNone = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	auto flagFastTrace = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	auto flagFastBuild = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
	auto flagAllowUpdate = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

	// Clear old instance lists
	for (auto& it : m_bottomBuffers[frameIndex]) {
		it.second.instanceList.clear();
	}
	//Destroy Metaball BLAS and instance lists
	for (auto& it : m_bottomBuffers_Metaballs[frameIndex]) {
		it.second.instanceList.clear();
		it.second.blas.release();
	}
	m_bottomBuffers_Metaballs[frameIndex].clear();

	// Iterate all static meshes
	for (auto& renderCommand : sceneGeometry) {
		if (renderCommand.type == Renderer::RENDER_COMMAND_TYPE_MODEL) {
			if (renderCommand.flags & Renderer::MESH_STATIC) {
				Mesh* mesh = renderCommand.model.mesh;			
				auto& searchResult = m_bottomBuffers[frameIndex].find(mesh);
				// If mesh does not have a BLAS
				if (searchResult == m_bottomBuffers[frameIndex].end()) {
					createBLAS(renderCommand, flagFastTrace, cmdList);
				} else {
					if (renderCommand.hasUpdatedSinceLastRender[frameIndex]) {
						SAIL_LOG_WARNING("A BLAS rebuild has been triggered on a STATIC mesh. Consider changing it to DYNAMIC!");
						// Destroy old blas
						searchResult->second.blas.release();
						m_bottomBuffers[frameIndex].erase(searchResult);
						// Create new one
						createBLAS(renderCommand, flagFastTrace, cmdList);
					} else {
						// Mesh already has a BLAS - add transform to instance list
						searchResult->second.instanceList.emplace_back(PerInstance{(glm::mat3x4)renderCommand.transform, (char)renderCommand.teamColorID , renderCommand.castShadows});
					}
				}
				totalNumInstances++;
			}
		}
	}

	// Iterate all dynamic meshes
	for (auto& renderCommand : sceneGeometry) {
		if (renderCommand.type == Renderer::RENDER_COMMAND_TYPE_MODEL) {
			if (renderCommand.flags & Renderer::MESH_DYNAMIC) {
				Mesh* mesh = renderCommand.model.mesh;
		
				auto& searchResult = m_bottomBuffers[frameIndex].find(mesh);
				auto flags = flagNone;
				if (renderCommand.flags & Renderer::MESH_HERO) {
					flags = flagFastTrace | flagAllowUpdate;
				} else {
					flags = flagFastBuild | flagAllowUpdate;
				}

				// If mesh does not have a BLAS or was first built as STATIC
				if (searchResult == m_bottomBuffers[frameIndex].end() || !searchResult->second.blas.allowUpdate) {
					createBLAS(renderCommand, flags, cmdList);
				} else {
					if (renderCommand.hasUpdatedSinceLastRender[frameIndex]) {
						createBLAS(renderCommand, flags, cmdList, &searchResult->second.blas);
					}
					// Add transform to instance list
					searchResult->second.instanceList.emplace_back(PerInstance{ (glm::mat3x4)renderCommand.transform, (char)renderCommand.teamColorID , renderCommand.castShadows});
				}

				totalNumInstances++;
			}
		}
	}

	// Iterate all metaball groups
	for (auto& group : metaballGroups) {

		Renderer::RenderCommand cmd;
		cmd.transform = glm::identity<glm::mat4>();
		cmd.transform = glm::transpose(cmd.transform);
		cmd.type = Renderer::RenderCommandType::RENDER_COMMAND_TYPE_NON_MODEL_METABALL;
		cmd.castShadows = true;
		cmd.teamColorID = 0;
		cmd.metaball.gpuGroupIndex = group->index;

		createBLAS(cmd, flagFastTrace, cmdList);
		totalNumInstances++;
	}

	// Destroy BLASes that are no longer part of the scene
	for (auto it = m_bottomBuffers[frameIndex].begin(); it != m_bottomBuffers[frameIndex].end();) {
		bool destroy = true;
		for (auto& renderCommand : sceneGeometry) {
			Mesh* mesh = nullptr;
			if (renderCommand.type == Renderer::RENDER_COMMAND_TYPE_MODEL) {
				mesh = renderCommand.model.mesh;
			}

			if (it->first == mesh) {
				destroy = false;
				++it;
				break;
			}
		}
		if (destroy) {
			it->second.blas.release();
			it = m_bottomBuffers[frameIndex].erase(it);
		}
	}

	createTLAS(totalNumInstances, cmdList);
	updateDescriptorHeap(cmdList);
	updateShaderTables();

}

void DXRBase::updateSceneData(Camera& cam, LightSetup& lights, const std::vector<DXRBase::MetaballGroup*>& metaballGroups, const std::vector<glm::vec3>& teamColors, bool doToneMapping) {

	updateMetaballpositions(metaballGroups);

	DXRShaderCommon::SceneCBuffer newData = {};
	newData.viewToWorld = glm::inverse(cam.getViewMatrix());
	newData.clipToView = glm::inverse(cam.getProjMatrix());
	newData.nearZ = cam.getNearZ();
	newData.farZ = cam.getFarZ();
	newData.cameraPosition = cam.getPosition();
	newData.cameraDirection = cam.getDirection();
	newData.projectionToWorld = glm::inverse(cam.getViewProjection());
	newData.nMetaballGroups = metaballGroups.size();

	for (auto& group : metaballGroups) {
		newData.metaballGroup[group->index].start = group->gpuGroupStartOffset;
		newData.metaballGroup[group->index].size = group->balls.size();
	}

	newData.nDecals = m_decalsToRender;
	newData.doTonemapping = doToneMapping;
	newData.waterArraySize = m_waterArrSizes;
	newData.mapSize = m_mapSize;
	newData.mapStart = m_mapStart;

	int nTeams = teamColors.size();
	for (int i = 0; i < nTeams && i < NUM_TEAM_COLORS; i++) {
		newData.teamColors[i] = glm::vec4(teamColors[i], 1.0f);
	}

	auto& plData = lights.getPointLightsData();
	memcpy(newData.pointLights, plData.pLights, sizeof(plData));

	auto& slData = lights.getSpotLightsData();
	memcpy(newData.spotLights, slData.sLights, sizeof(slData));

	m_sceneCB->updateData(&newData, sizeof(newData));
}

void DXRBase::updateDecalData(DXRShaderCommon::DecalData* decals, size_t size) {
	DXRShaderCommon::DecalCBuffer newData;
	memcpy(newData.data, decals, size * sizeof(DXRShaderCommon::DecalData));
	m_decalsToRender = size;

	m_decalCB->updateData(&newData, sizeof(newData));
}

void DXRBase::addWaterAtWorldPosition(const glm::vec3& position) {
	static auto& mapSettings = Application::getInstance()->getSettings().gameSettingsDynamic["map"];
	auto mapSize = glm::vec3(mapSettings["sizeX"].value, 0.8f, mapSettings["sizeY"].value) * (float)mapSettings["tileSize"].value;
	auto mapStart = -glm::vec3((float)mapSettings["tileSize"].value / 2.0f, 0.f, (float)mapSettings["tileSize"].value / 2.0f);

	// Convert position to index, stored as floats
	glm::vec3 floatInd = ((position - mapStart) / mapSize) * m_waterArrSizes;
	// Convert triple-number index to a single index value
	int quarterIndex = glm::floor((int)glm::floor(floatInd.x * 4.f) % 4);
	// Convert triple-number (float) to triple-number (int)
	glm::i32vec3 ind = floor(floatInd);
	// Convert to final 1-D index
	int arrIndex = Utils::to1D(ind, m_waterArrSizes.x, m_waterArrSizes.y);

	// Ignore water points that are outside the map
	if (arrIndex >= 0 && arrIndex <= m_waterArrSize - 1) {
		// Make sure to update this water
		m_updateWater[arrIndex] = true;
		uint8_t up0 = Utils::unpackQuarterFloat(m_waterDataCPU[arrIndex], 0);
		uint8_t up1 = Utils::unpackQuarterFloat(m_waterDataCPU[arrIndex], 1);
		uint8_t up2 = Utils::unpackQuarterFloat(m_waterDataCPU[arrIndex], 2);
		uint8_t up3 = Utils::unpackQuarterFloat(m_waterDataCPU[arrIndex], 3);

		switch (quarterIndex) {
		case 0:
			m_waterDeltas[arrIndex] = Utils::packQuarterFloat(std::min(255U, up0 + std::rand() % 50U + 50U), up1, up2, up3);
			break;
		case 1:
			m_waterDeltas[arrIndex] = Utils::packQuarterFloat(up0, std::min(255U, up1 + std::rand() % 50U + 50U), up2, up3);
			break;
		case 2:
			m_waterDeltas[arrIndex] = Utils::packQuarterFloat(up0, up1, std::min(255U, up2 + std::rand() % 50U + 50U), up3);
			break;
		case 3:
			m_waterDeltas[arrIndex] = Utils::packQuarterFloat(up0, up1, up2, std::min(255U, up3 + std::rand() % 50U + 50U));
			break;
		}

		m_waterDataCPU[arrIndex] = m_waterDeltas[arrIndex];
		m_waterChanged = true;
	}
}

bool DXRBase::checkWaterAtWorldPosition(const glm::vec3& position) {
	bool returnValue = false;

	static auto& mapSettings = Application::getInstance()->getSettings().gameSettingsDynamic["map"];
	auto mapSize = glm::vec3(mapSettings["sizeX"].value, 0.8f, mapSettings["sizeY"].value) * (float)mapSettings["tileSize"].value;
	auto mapStart = -glm::vec3((float)mapSettings["tileSize"].value / 2.0f, 0.f, (float)mapSettings["tileSize"].value / 2.0f);

	// Convert position to index, stored as floats
	glm::vec3 floatInd = ((position - mapStart) / mapSize) * m_waterArrSizes;
	// Convert triple-number (float) to triple-number (int)
	glm::i32vec3 ind = floor(floatInd);
	// Convert to final 1-D index
	int arrIndex = Utils::to1D(ind, m_waterArrSizes.x, m_waterArrSizes.y);

	// Ignore water points that are outside the map
	if (arrIndex >= 0 && arrIndex <= m_waterArrSize - 1) {
		returnValue = m_waterDataCPU[arrIndex] != 0;
	}

	return returnValue;
}

void DXRBase::updateWaterData() {
	if (Application::getInstance()->getSettings().applicationSettingsStatic["graphics"]["watersimulation"].getSelected().value) {
		simulateWater(Application::getInstance()->getDelta());
	}

	for (auto& pair : m_waterDeltas) {
		unsigned int offset = sizeof(float) * pair.first;
		unsigned int& data = pair.second;
		m_waterStructuredBuffer->updateData_new(&data, 1, 0, offset);
	}

	// Reset every other frame because of double buffering
	if (!m_waterChanged) {
		m_waterDeltas.clear();
	}
	m_waterChanged = false;
}

void DXRBase::simulateWater(float dt) {
	for (unsigned int z = 0; z < m_waterArrSizes.z; z++) {
		for (unsigned int y = 1; y < m_waterArrSizes.y; y++) {
			for (unsigned int x = 0; x < m_waterArrSizes.x; x++) {
				auto arrIndex = Utils::to1D(glm::i32vec3(x, y, z), m_waterArrSizes.x, m_waterArrSizes.y);
				if (m_updateWater[arrIndex]) {
					unsigned int arrIndexBelow = arrIndex - m_waterArrSizes.x;

					bool change = false;
					uint32_t vals[8];
					bool keepUpdating = false;
					for (unsigned int quarterIndex = 0; quarterIndex < 4; quarterIndex++) {
						int quarterVal = Utils::unpackQuarterFloat(m_waterDataCPU[arrIndex], quarterIndex);
						uint32_t belowQuarterVal = Utils::unpackQuarterFloat(m_waterDataCPU[arrIndexBelow], quarterIndex);

						// Below
						int deltaVal = 0;

						deltaVal = static_cast<int>(7.f * ((float)(quarterVal) / 255.f) + 1.f);

						if (quarterVal < 21) {
							deltaVal = quarterVal;
						}
						quarterVal -= deltaVal;
						if (quarterVal > 0) {
							keepUpdating = true;
						}
						vals[quarterIndex] = static_cast<uint32_t>(quarterVal);
						belowQuarterVal += deltaVal;
						if (belowQuarterVal > 100U) {
							m_updateWater[arrIndexBelow] = true;
						}
						vals[quarterIndex + 4] = std::min(belowQuarterVal, 255U);
					}

					if (!keepUpdating) {
						m_updateWater[arrIndex] = false;
					}

					m_waterDeltas[arrIndex] = Utils::packQuarterFloat(vals[0], vals[1], vals[2], vals[3]);
					m_waterDeltas[arrIndexBelow] = Utils::packQuarterFloat(vals[4], vals[5], vals[6], vals[7]);
					m_waterDataCPU[arrIndex] = m_waterDeltas[arrIndex];
					m_waterDataCPU[arrIndexBelow] = m_waterDeltas[arrIndexBelow];
					m_waterChanged = true;
				}
			}
		}
	}
}

void DXRBase::rebuildWater() {
	// Get some map settings
	auto& mapSettings = Application::getInstance()->getSettings().gameSettingsDynamic["map"];
	m_mapSize = glm::vec3(mapSettings["sizeX"].value, 0.8f, mapSettings["sizeY"].value) * (float)mapSettings["tileSize"].value;
	m_mapStart = -glm::vec3((float)mapSettings["tileSize"].value / 2.0f, 0.f, (float)mapSettings["tileSize"].value / 2.0f);
	auto tileSize = mapSettings["tileSize"].value;
	float mapSizeX = mapSettings["sizeX"].value * tileSize;
	float mapSizeZ = mapSettings["sizeY"].value * tileSize;

	// Water voxel apperance setting, increase to improve water resolution
	const float voxelCellsPerWorldUnit = 5.f;
	// How many values are packed into each X, don't change this
	const int valuesPerX = 4;

	m_waterArrSizes.x = glm::floor(mapSizeX * voxelCellsPerWorldUnit / valuesPerX);
	m_waterArrSizes.y = 37;
	m_waterArrSizes.z = glm::floor(mapSizeZ * voxelCellsPerWorldUnit);
	
	m_waterArrSize = m_waterArrSizes.x * m_waterArrSizes.y * m_waterArrSizes.z;

	// Init water "decals"
	unsigned int numElements = m_waterArrSize;
	Memory::SafeDeleteArr(m_waterDataCPU);
	Memory::SafeDeleteArr(m_updateWater);
	m_waterDataCPU = SAIL_NEW unsigned int[numElements];
	m_updateWater = SAIL_NEW bool[numElements];
	memset(m_waterDataCPU, 0, sizeof(unsigned int) * numElements);
	memset(m_updateWater, 0, sizeof(bool) * numElements);
	// Recreate sbuffer to resize it
	m_waterStructuredBuffer = std::make_unique<ShaderComponent::DX12StructuredBuffer>(m_waterDataCPU, numElements, sizeof(unsigned int));
	// Stop any current changes
	m_waterDeltas.clear();

	// Reset simulation data
	m_currWaterZChunk = 0;
	m_maxWaterZChunk = 5;
	m_waterZChunkSize = m_waterArrSizes.z / m_maxWaterZChunk;
}

void DXRBase::updateMetaballpositions(const std::vector<DXRBase::MetaballGroup*>& metaballGroups) {
	if (metaballGroups.empty()) {
		return;
	}

	HRESULT hr;

	void* pAabbMappedData;
	ID3D12Resource1* aabb_res;

	void* pPosMappedData;
	ID3D12Resource1* pos_res = m_metaballPositions_srv[m_context->getSwapIndex()];
 	hr = pos_res->Map(0, nullptr, &pPosMappedData);
	DX12Utils::checkDeviceRemovalReason(m_context->getDevice(), hr);

	int metaballIndex = 0;
	int metaballOffsetBytes = 0;

	int offsetInc = sizeof(Metaball::pos);
	int bufferMaxSize = sizeof(Metaball::pos) * MAX_NUM_METABALLS;
	
	for (auto group : metaballGroups) {
		metaballOffsetBytes = group->gpuGroupStartOffset * offsetInc;

		if (!m_aabb_desc_resources.count(group->index)) {
			addMetaballGroupAABB(group->index);
		}

		//UPDATE AABB
		aabb_res = m_aabb_desc_resources[group->index][m_context->getSwapIndex()];
		hr = aabb_res->Map(0, nullptr, &pAabbMappedData);
		DX12Utils::checkDeviceRemovalReason(m_context->getDevice(), hr);

		memcpy(pAabbMappedData, &group->aabb, sizeof(D3D12_RAYTRACING_AABB));
		aabb_res->Unmap(0, nullptr);

		//UPDATE METABALLS
		const std::vector<Metaball>& metaballs = group->balls;
		size_t size = metaballs.size();

		for (size_t i = 0; i < size && metaballOffsetBytes < bufferMaxSize; i++) {;
			memcpy(static_cast<char*>(pPosMappedData) + metaballOffsetBytes, &metaballs[i].pos, offsetInc);
			metaballOffsetBytes += offsetInc;
		}		
	}
	pos_res->Unmap(0, nullptr);
}

void DXRBase::dispatch(DX12RenderableTexture* outputTexture, DX12RenderableTexture* outputBloomTexture, ID3D12GraphicsCommandList4* cmdList) {

	assert(m_gbufferInputTextures); // Input textures not set!
	
	unsigned int frameIndex = m_context->getSwapIndex();

	auto copyDescriptor = [&](DX12RenderableTexture* texture, D3D12_CPU_DESCRIPTOR_HANDLE* cdh) {
		// Copy output texture uav to heap
		texture->transitionStateTo(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		m_context->getDevice()->CopyDescriptorsSimple(1, cdh[frameIndex], texture->getUavCDH(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	};
	copyDescriptor(outputTexture, m_rtOutputTextureUavCPUHandles);
	copyDescriptor(outputBloomTexture, m_rtOutputBloomTextureUavCPUHandles);

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
	cmdList->SetComputeRootConstantBufferView(m_dxrGlobalRootSignature->getIndex("SceneCBuffer"), m_sceneCB->getBuffer()->GetGPUVirtualAddress());
	// Set water "decal" data
	cmdList->SetComputeRootShaderResourceView(m_dxrGlobalRootSignature->getIndex("WaterData"), m_waterStructuredBuffer->getBuffer()->GetGPUVirtualAddress());

	// Dispatch
	cmdList->SetPipelineState1(m_rtPipelineState.Get());
	cmdList->DispatchRays(&raytraceDesc);
}

void DXRBase::resetWater() {
	// TODO: make faster by updates the whole buffer at once
	for (unsigned int i = 0; i < m_waterArrSize; i++) {
		m_waterDataCPU[i] = m_waterDeltas[i] = 0;
	}
	m_waterChanged = true;
}

void DXRBase::reloadShaders() {
	m_context->waitForGPU();
	// Recompile hlsl
	createRaytracingPSO();
}

bool DXRBase::onEvent(const Event& event) {
	auto onResize = [&](const WindowResizeEvent& event) {
		// Window changed size, resize output UAV
		createInitialShaderResources(true);
		return true;
	};

	auto onResetWater = [&](const ResetWaterEvent& event) {
		resetWater();
		return true;
	};

	switch (event.type) {
	case Event::Type::WINDOW_RESIZE: onResize((const WindowResizeEvent&)event); break;
	case Event::Type::RESET_WATER: onResetWater((const ResetWaterEvent&)event); break;
	default: break;
	}
	return true;
}

void DXRBase::createTLAS(unsigned int numInstanceDescriptors, ID3D12GraphicsCommandList4* cmdList) {

	// Always rebuilds TLAS instead of updating it according to nvidia recommendations

	unsigned int frameIndex = m_context->getSwapIndex();

	// First, get the size of the TLAS buffers and create them
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = numInstanceDescriptors;
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

	m_DXR_TopBuffer[frameIndex].instanceDesc = DX12Utils::CreateBuffer(m_context->getDevice(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * glm::max(numInstanceDescriptors, 1U), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, DX12Utils::sUploadHeapProperties);
	m_DXR_TopBuffer[frameIndex].instanceDesc->SetName(L"TLAS_INSTANCE_DESC");

	D3D12_RAYTRACING_INSTANCE_DESC* pInstanceDesc;
	m_DXR_TopBuffer[frameIndex].instanceDesc->Map(0, nullptr, (void**)& pInstanceDesc);

	unsigned int blasIndex = 0;
	unsigned int instanceID = 0;

	for (auto& it : m_bottomBuffers[frameIndex]) {
		auto& instanceList = it.second;
		for (auto& instance : instanceList.instanceList) {

#ifdef DEVELOPMENT
			if (blasIndex >= 1 << 10) {
				SAIL_LOG_WARNING("BlasIndex is to high and will interfere with team color index.");
			}
#endif
			pInstanceDesc->InstanceID = blasIndex | (instance.teamColorIndex << 10);
			UINT shadowMask = (instance.castShadows) ? INSTANCE_MASK_CAST_SHADOWS : 0;
			pInstanceDesc->InstanceMask = INSTANCE_MASK_DEFAULT | shadowMask;
			
			pInstanceDesc->InstanceContributionToHitGroupIndex = blasIndex * 2;	// offset inside the shader-table. Unique for every instance since each geometry has different vertexbuffer/indexbuffer/textures
																				// * 2 since every other entry in the SBT is for shadow rays (NULL hit group)
			pInstanceDesc->Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

			memcpy(pInstanceDesc->Transform, &instance.transform, sizeof(pInstanceDesc->Transform));

			pInstanceDesc->AccelerationStructure = instanceList.blas.result->GetGPUVirtualAddress();

			pInstanceDesc++;
		}
		blasIndex++;
	}

	//Metaballs
	for (auto& it : m_bottomBuffers_Metaballs[frameIndex]) {
		auto& instanceList = it.second;
		for (auto& instance : instanceList.instanceList) {
			pInstanceDesc->InstanceID = it.first;								// exposed to the shader via InstanceID() - currently same for all instances of same material
			pInstanceDesc->InstanceMask = INSTANCE_MASK_METABALLS;
			pInstanceDesc->InstanceContributionToHitGroupIndex = blasIndex * 2;	// offset inside the shader-table. Unique for every instance since each geometry has different vertexbuffer/indexbuffer/textures
																				// * 2 since every other entry in the SBT is for shadow rays (NULL hit group)
			pInstanceDesc->Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

			memcpy(pInstanceDesc->Transform, &instance.transform, sizeof(pInstanceDesc->Transform));

			pInstanceDesc->AccelerationStructure = instanceList.blas.result->GetGPUVirtualAddress();

			pInstanceDesc++;
		}
		blasIndex++;
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

void DXRBase::createBLAS(const Renderer::RenderCommand& renderCommand, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags, ID3D12GraphicsCommandList4* cmdList, AccelerationStructureBuffers* sourceBufferForUpdate) {
	unsigned int frameIndex = m_context->getSwapIndex();
	Mesh* mesh = nullptr; 
	if (renderCommand.type == Renderer::RENDER_COMMAND_TYPE_MODEL) {
		mesh = renderCommand.model.mesh;
	}

	bool performInplaceUpdate = (sourceBufferForUpdate) ? true : false;

	InstanceList instance;
	instance.instanceList.emplace_back(PerInstance{ renderCommand.transform , (char)renderCommand.teamColorID, renderCommand.castShadows});
	AccelerationStructureBuffers& bottomBuffer = instance.blas;
	if (performInplaceUpdate) {
		bottomBuffer = *sourceBufferForUpdate;
	}
	if (flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE) {
		instance.blas.allowUpdate = true;
	}

	D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
	if (renderCommand.type == Renderer::RENDER_COMMAND_TYPE_MODEL) {
		auto& vb = static_cast<DX12VertexBuffer&>(mesh->getVertexBuffer());
		auto& ib = static_cast<const DX12IndexBuffer&>(mesh->getIndexBuffer());

		// Make sure vbuffer is initialized
		vb.init(cmdList);

		geomDesc.Flags = (renderCommand.flags & Renderer::MESH_TRANSPARENT) ? D3D12_RAYTRACING_GEOMETRY_FLAG_NONE : D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geomDesc.Triangles.VertexBuffer.StartAddress = vb.getBuffer()->GetGPUVirtualAddress();
		geomDesc.Triangles.VertexBuffer.StrideInBytes = vb.getVertexDataStride();
		geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		geomDesc.Triangles.VertexCount = mesh->getNumVertices();

		if (mesh->getNumIndices() > 0) {
			geomDesc.Triangles.IndexBuffer = ib.getBuffer()->GetGPUVirtualAddress();
			geomDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
			geomDesc.Triangles.IndexCount = UINT(mesh->getNumIndices());
		}
	} else if(renderCommand.type == Renderer::RENDER_COMMAND_TYPE_NON_MODEL_METABALL){
		//No mesh included. Use AABB from GPU memmory (m_aabb_desc_resource) and set type to PROCEDURAL_PRIMITIVE.
		geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
		geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
		geomDesc.AABBs.AABBCount = 1;
		geomDesc.AABBs.AABBs.StartAddress = m_aabb_desc_resources[renderCommand.metaball.gpuGroupIndex][m_context->getSwapIndex()]->GetGPUVirtualAddress();
		geomDesc.AABBs.AABBs.StrideInBytes = 0;
	}

	// Get the size requirements for the scratch and AS buffers
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = flags; // Changing this flag depending on mesh can speed up performance significantly!
	if (performInplaceUpdate) {
		inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
	}
	inputs.NumDescs = 1;
	inputs.pGeometryDescs = &geomDesc;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
	m_context->getDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	// TODO: make sure buffer size is >= info.UpdateScratchDataSize in bytes
	if (!performInplaceUpdate) {
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
	if (inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE) {
		asDesc.SourceAccelerationStructureData = sourceBufferForUpdate->result->GetGPUVirtualAddress();
	}

	cmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = bottomBuffer.result.Get();
	cmdList->ResourceBarrier(1, &uavBarrier);

	if (!performInplaceUpdate) {
		// Insert BLAS into buttom buffer map
		if (renderCommand.type == Renderer::RenderCommandType::RENDER_COMMAND_TYPE_MODEL) {
			m_bottomBuffers[frameIndex].insert({ mesh, instance });
		} else if (renderCommand.type == Renderer::RenderCommandType::RENDER_COMMAND_TYPE_NON_MODEL_METABALL) {
			m_bottomBuffers_Metaballs[frameIndex].insert({ renderCommand.metaball.gpuGroupIndex, instance });
		}
	}
}

void DXRBase::createInitialShaderResources(bool remake) {
	initMetaballBuffers();

	// Create some resources only once on init
	if (!m_rtDescriptorHeap || remake) {
		m_rtDescriptorHeap.Reset();
		UINT numDescriptors = 5000; // TODO: this does not throw error when full
		m_usedDescriptors = 0;

		D3D12_DESCRIPTOR_HEAP_DESC heapDescriptorDesc = {};
		heapDescriptorDesc.NumDescriptors = numDescriptors;
		heapDescriptorDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDescriptorDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		m_context->getDevice()->CreateDescriptorHeap(&heapDescriptorDesc, IID_PPV_ARGS(&m_rtDescriptorHeap));

		m_heapIncr = m_context->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// Create the UAV. Based on the root signature we created it should be the first entry
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_rtDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_rtDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

		auto storeHandle = [&](D3D12_CPU_DESCRIPTOR_HANDLE* cdh, D3D12_GPU_DESCRIPTOR_HANDLE* gdh) {
			gdh[0] = gpuHandle;
			cdh[0] = cpuHandle;
			cpuHandle.ptr += m_heapIncr;
			gpuHandle.ptr += m_heapIncr;
			m_usedDescriptors++;
			gdh[1] = gpuHandle;
			cdh[1] = cpuHandle;
			cpuHandle.ptr += m_heapIncr;
			gpuHandle.ptr += m_heapIncr;
			m_usedDescriptors++;
		};

		// The first slots in the heap will be used for the output UAV
		storeHandle(m_rtOutputTextureUavCPUHandles, m_rtOutputTextureUavGPUHandles);
		storeHandle(m_rtOutputBloomTextureUavCPUHandles, m_rtOutputBloomTextureUavGPUHandles);

		// Next slot is used for the brdfLUT
		m_rtBrdfLUTGPUHandle = gpuHandle;
		if (!Application::getInstance()->getResourceManager().hasTexture(m_brdfLUTPath)) {
			Application::getInstance()->getResourceManager().loadTexture(m_brdfLUTPath);
		}
		auto& brdfLutTex = static_cast<DX12Texture&>(Application::getInstance()->getResourceManager().getTexture(m_brdfLUTPath));
		m_context->getDevice()->CopyDescriptorsSimple(1, cpuHandle, brdfLutTex.getSrvCDH(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		cpuHandle.ptr += m_heapIncr;
		gpuHandle.ptr += m_heapIncr;
		m_usedDescriptors++;

		// Next (4 * numSwapBuffers) slots are used for input gbuffers
		for (unsigned int i = 0; i < m_context->getNumGPUBuffers(); i++) {
			m_gbufferStartGPUHandles[i] = gpuHandle;
			D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptors[4];
			srcDescriptors[0] = m_gbufferInputTextures[0]->getSrvCDH(i);
			srcDescriptors[1] = m_gbufferInputTextures[1]->getSrvCDH(i);
			srcDescriptors[2] = m_gbufferInputTextures[2]->getSrvCDH(i);
			srcDescriptors[3] = m_gbufferInputTextures[0]->getDepthSrvCDH(i);

			UINT dstRangeSizes[] = { 4 };
			UINT srcRangeSizes[] = { 1, 1, 1, 1 };
			m_context->getDevice()->CopyDescriptors(1, &cpuHandle, dstRangeSizes, 4, srcDescriptors, srcRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			cpuHandle.ptr += m_heapIncr * 4;
			gpuHandle.ptr += m_heapIncr * 4;
			m_usedDescriptors+=4;
		}

		// Initialize decal SRVs
		initDecals(&gpuHandle, &cpuHandle);

		//// Ray gen settings CB
		//m_rayGenCBData.flags = RT_ENABLE_TA | RT_ENABLE_JITTER_AA;
		//m_rayGenCBData.numAORays = 5;
		//m_rayGenCBData.AORadius = 0.9f;
		//m_rayGenCBData.frameCount = 0;
		//m_rayGenCBData.GISamples = 1;
		//m_rayGenCBData.GIBounces = 1;
		//m_rayGenSettingsCB = std::make_unique<DX12ConstantBuffer>("Ray Gen Settings CB", sizeof(RayGenSettings), m_renderer);
		//m_rayGenSettingsCB->setData(&m_rayGenCBData, 0);

		// Store heap start for views that might update in runtime
		// Half of the rest of the list is allocated for each swap frame
		m_rtHeapCPUHandle[0] = cpuHandle;
		m_rtHeapGPUHandle[0] = gpuHandle;

		auto frameDescriptorSlots = (numDescriptors - m_usedDescriptors) / 2;
		cpuHandle.ptr += m_heapIncr * frameDescriptorSlots;
		gpuHandle.ptr += m_heapIncr * frameDescriptorSlots;
		m_rtHeapCPUHandle[1] = cpuHandle;
		m_rtHeapGPUHandle[1] = gpuHandle;

		// Scene CB
		{
			unsigned int size = sizeof(DXRShaderCommon::SceneCBuffer);
			void* initData = malloc(size);
			memset(initData, 0, size);
			m_sceneCB = std::make_unique<ShaderComponent::DX12ConstantBuffer>(initData, size, ShaderComponent::BIND_SHADER::CS, 0);
			free(initData);
		}
		// Mesh CB
		{
			unsigned int size = sizeof(DXRShaderCommon::MeshCBuffer);
			void* initData = malloc(size);
			memset(initData, 0, size);
			m_meshCB = std::make_unique<ShaderComponent::DX12ConstantBuffer>(initData, size, ShaderComponent::BIND_SHADER::CS, 0);
			free(initData);
		}
		// Decal CB
		{
			unsigned int size = sizeof(DXRShaderCommon::DecalCBuffer);
			void* initData = malloc(size);
			memset(initData, 0, size);
			m_decalCB = std::make_unique<ShaderComponent::DX12ConstantBuffer>(initData, size, ShaderComponent::BIND_SHADER::CS, 0);
			free(initData);
		}
	}

}

void DXRBase::updateDescriptorHeap(ID3D12GraphicsCommandList4* cmdList) {
	unsigned int frameIndex = m_context->getSwapIndex();

	// Make sure brdfLut texture has been initialized
	auto& brdfLutTex = static_cast<DX12Texture&>(Application::getInstance()->getResourceManager().getTexture(m_brdfLUTPath));
	brdfLutTex.initBuffers(cmdList);

	// Make sure decal textures has been initialized
	{
		auto& decalTex = static_cast<DX12Texture&>(Application::getInstance()->getResourceManager().getTexture(m_decalTexPaths[0]));
		if (!decalTex.hasBeenInitialized()) {
			decalTex.initBuffers(cmdList);
			decalTex.transitionStateTo(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}
		auto& decalTex1 = static_cast<DX12Texture&>(Application::getInstance()->getResourceManager().getTexture(m_decalTexPaths[1]));
		if (!decalTex1.hasBeenInitialized()) {
			decalTex1.initBuffers(cmdList);
			decalTex1.transitionStateTo(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}
		auto& decalTex2 = static_cast<DX12Texture&>(Application::getInstance()->getResourceManager().getTexture(m_decalTexPaths[2]));
		if (!decalTex2.hasBeenInitialized()) {
			decalTex2.initBuffers(cmdList);
			decalTex2.transitionStateTo(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}
	}

	// Update descriptors for vertices, indices, textures etc
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_rtHeapCPUHandle[frameIndex];
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_rtHeapGPUHandle[frameIndex];
	m_rtMeshHandles[frameIndex].clear();

	unsigned int blasIndex = 0;
	unsigned int metaballIndex = 0;
	for (auto& it : m_bottomBuffers[frameIndex]) {
		auto& instanceList = it.second;
		Mesh* mesh = it.first;

		unsigned int meshDataSize = sizeof(DXRShaderCommon::MeshData);
		DXRShaderCommon::MeshData meshData;
		
		MeshHandles handles;
		if (mesh) {
			handles.vertexBufferHandle = static_cast<const DX12VertexBuffer&>(mesh->getVertexBuffer()).getBuffer()->GetGPUVirtualAddress();
			if (mesh->getNumIndices() > 0) {
				handles.indexBufferHandle = static_cast<const DX12IndexBuffer&>(mesh->getIndexBuffer()).getBuffer()->GetGPUVirtualAddress();
			}

			auto& materialSettings = mesh->getMaterial()->getPBRSettings();

			// Three textures
			for (unsigned int textureNum = 0; textureNum < 3; textureNum++) {
				DX12Texture* texture = static_cast<DX12Texture*>(mesh->getMaterial()->getTexture(textureNum));
				bool hasTexture = (textureNum == 0) ? materialSettings.hasAlbedoTexture : materialSettings.hasNormalTexture;
				hasTexture = (textureNum == 2) ? materialSettings.hasMetalnessRoughnessAOTexture : hasTexture;
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
			meshData.flags |= (materialSettings.hasAlbedoTexture) ? DXRShaderCommon::MESH_HAS_ALBEDO_TEX : meshData.flags;
			meshData.flags |= (materialSettings.hasNormalTexture) ? DXRShaderCommon::MESH_HAS_NORMAL_TEX : meshData.flags;
			meshData.flags |= (materialSettings.hasMetalnessRoughnessAOTexture) ? DXRShaderCommon::MESH_HAS_METALNESS_ROUGHNESS_AO_TEX : meshData.flags;
			meshData.color = materialSettings.modelColor;
			meshData.metalnessRoughnessAoScales.r = materialSettings.metalnessScale;
			meshData.metalnessRoughnessAoScales.g = materialSettings.roughnessScale;
			meshData.metalnessRoughnessAoScales.b = materialSettings.aoScale;
			m_meshCB->updateData(&meshData, meshDataSize, blasIndex * meshDataSize);

			m_rtMeshHandles[frameIndex].emplace_back(handles);
		} else {
			m_rtMeshHandles[frameIndex].emplace_back(handles);
			static float time = 0.f;
			static float totalTime = 0.f;
			static float inc = 0.001f;
			time += inc;
			totalTime += abs(inc);

			if (time > 1.f) {
				time = 1.f;
				inc *= -1.f;
			} else if(time < 0.f) {
				time = 0.f;
				inc *= -1.f;
			}

			//float r = ((int)time % 10) / 10.0f;

			meshData.flags = DXRShaderCommon::MESH_NO_FLAGS;
			meshData.color = glm::vec4((float)(metaballIndex++), 1.f - time, totalTime, 1.f);
			m_meshCB->updateData(&meshData, meshDataSize, blasIndex * meshDataSize);
		}

		blasIndex++;
	}
}

void DXRBase::updateShaderTables() {

	// 	 "Shader tables can be modified freely by the application (with appropriate state barriers)"

	auto frameIndex = m_context->getSwapIndex();

	// Ray gen
	{
		if (m_rayGenShaderTable[frameIndex].Resource) {
			m_rayGenShaderTable[frameIndex].Resource->Release();
			m_rayGenShaderTable[frameIndex].Resource.Reset();
		}
		DXRUtils::ShaderTableBuilder tableBuilder(1U, m_rtPipelineState.Get(), 64U);
		tableBuilder.addShader(m_rayGenName);
		tableBuilder.addDescriptor(m_rtOutputTextureUavGPUHandles[frameIndex].ptr);
		tableBuilder.addDescriptor(m_rtOutputBloomTextureUavGPUHandles[frameIndex].ptr);
		tableBuilder.addDescriptor(m_gbufferStartGPUHandles[frameIndex].ptr);
		tableBuilder.addDescriptor(m_decalTexGPUHandles.ptr);
		tableBuilder.addDescriptor(m_rtBrdfLUTGPUHandle.ptr);
		D3D12_GPU_VIRTUAL_ADDRESS decalCBHandle = m_decalCB->getBuffer()->GetGPUVirtualAddress();
		tableBuilder.addDescriptor(decalCBHandle);
		m_rayGenShaderTable[frameIndex] = tableBuilder.build(m_context->getDevice());
	}

	// Miss
	{
		if (m_missShaderTable[frameIndex].Resource) {
			m_missShaderTable[frameIndex].Resource->Release();
			m_missShaderTable[frameIndex].Resource.Reset();
		}

		DXRUtils::ShaderTableBuilder tableBuilder(2U, m_rtPipelineState.Get());
		tableBuilder.addShader(m_missName);
		tableBuilder.addShader(m_shadowMissName);
		m_missShaderTable[frameIndex] = tableBuilder.build(m_context->getDevice());
	}

	// Hit group
	{
		if (m_hitGroupShaderTable[frameIndex].Resource) {
			m_hitGroupShaderTable[frameIndex].Resource->Release();
			m_hitGroupShaderTable[frameIndex].Resource.Reset();
		}

		UINT nInstances = ((UINT)m_bottomBuffers[frameIndex].size() + (UINT)m_bottomBuffers_Metaballs[frameIndex].size()) * 2U; /* * 2 for shadow rays (all NULL) */
		DXRUtils::ShaderTableBuilder tableBuilder(nInstances, m_rtPipelineState.Get(), 64U);

		unsigned int blasIndex = 0;

		for (auto& it : m_bottomBuffers[frameIndex]) {
			auto& instanceList = it.second;
			Mesh* mesh = it.first;

			tableBuilder.addShader(m_hitGroupTriangleName);//Set the shadergroup to use
			m_localSignatureHitGroup_mesh->doInOrder([&](const std::string& parameterName) {
				if (parameterName == "VertexBuffer") {
					tableBuilder.addDescriptor(m_rtMeshHandles[frameIndex][blasIndex].vertexBufferHandle, blasIndex * 2);
				} else if (parameterName == "IndexBuffer") {
					D3D12_GPU_VIRTUAL_ADDRESS nullAddr = 0;
					tableBuilder.addDescriptor((mesh->getNumIndices() > 0) ? m_rtMeshHandles[frameIndex][blasIndex].indexBufferHandle : nullAddr, blasIndex * 2);
				} else if (parameterName == "MeshCBuffer") {
					D3D12_GPU_VIRTUAL_ADDRESS meshCBHandle = m_meshCB->getBuffer()->GetGPUVirtualAddress();
					tableBuilder.addDescriptor(meshCBHandle, blasIndex * 2);
				} else if (parameterName == "Textures") {
					// Three textures
					for (unsigned int textureNum = 0; textureNum < 3; textureNum++) {
						tableBuilder.addDescriptor(m_rtMeshHandles[frameIndex][blasIndex].textureHandles[textureNum].ptr, blasIndex * 2);
					}
				} else if (parameterName == "sys_brdfLUT") {
					tableBuilder.addDescriptor(m_rtBrdfLUTGPUHandle.ptr, blasIndex * 2);
				} else {
					SAIL_LOG_ERROR("Unhandled root signature parameter! (" + parameterName + ")");
				}

			});
			
			tableBuilder.addShader(L"NULL");
			blasIndex++;
		}

		for (auto& it : m_bottomBuffers_Metaballs[frameIndex]) {
			tableBuilder.addShader(m_hitGroupMetaBallName);//Set the shadergroup to use
			m_localSignatureHitGroup_metaball->doInOrder([&](const std::string& parameterName) {
				if (parameterName == "MeshCBuffer") {
					D3D12_GPU_VIRTUAL_ADDRESS meshCBHandle = m_meshCB->getBuffer()->GetGPUVirtualAddress();
					tableBuilder.addDescriptor(meshCBHandle, blasIndex * 2);
				} else if (parameterName == "MetaballPositions") {
					D3D12_GPU_VIRTUAL_ADDRESS metaballHandle = m_metaballPositions_srv[frameIndex]->GetGPUVirtualAddress();
					tableBuilder.addDescriptor(metaballHandle, blasIndex * 2);
				} else if (parameterName == "sys_brdfLUT") {
					tableBuilder.addDescriptor(m_rtBrdfLUTGPUHandle.ptr, blasIndex * 2);
				} else {
					SAIL_LOG_ERROR("Unhandled root signature parameter! (" + parameterName + ")");
				}
			});

			tableBuilder.addShader(L"NULL");
			blasIndex++;
		}

		m_hitGroupShaderTable[frameIndex] = tableBuilder.build(m_context->getDevice());
	}
}

void DXRBase::createRaytracingPSO() {
	Memory::SafeRelease(m_rtPipelineState);

	DXRUtils::PSOBuilder psoBuilder;

	psoBuilder.addLibrary(ShaderPipeline::DEFAULT_SHADER_LOCATION + "dxr/" + m_shaderFilename + ".hlsl", { m_rayGenName, m_closestHitName, m_missName, m_closestProceduralPrimitive, m_intersectionProceduralPrimitive });
	psoBuilder.addHitGroup(m_hitGroupTriangleName, m_closestHitName);
	psoBuilder.addHitGroup(m_hitGroupMetaBallName, m_closestProceduralPrimitive, nullptr, m_intersectionProceduralPrimitive, D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE); //TODO: Add intesection Shader here!

	psoBuilder.addSignatureToShaders({ m_rayGenName }, m_localSignatureRayGen->get());
	psoBuilder.addSignatureToShaders({ m_hitGroupTriangleName }, m_localSignatureHitGroup_mesh->get());
	psoBuilder.addSignatureToShaders({ m_hitGroupMetaBallName }, m_localSignatureHitGroup_metaball->get());
	psoBuilder.addSignatureToShaders({ m_missName }, m_localSignatureMiss->get());

	psoBuilder.addLibrary(ShaderPipeline::DEFAULT_SHADER_LOCATION + "dxr/ShadowRay.hlsl", { m_shadowMissName });
	psoBuilder.addSignatureToShaders({ m_shadowMissName }, m_localSignatureEmpty->get());

	psoBuilder.setMaxPayloadSize(sizeof(RayPayload));
	psoBuilder.setMaxAttributeSize(sizeof(float) * 4);
	psoBuilder.setMaxRecursionDepth(MAX_RAY_RECURSION_DEPTH);
	psoBuilder.setGlobalSignature(m_dxrGlobalRootSignature->get());

	m_rtPipelineState = psoBuilder.build(m_context->getDevice());
}

void DXRBase::createDXRGlobalRootSignature() {
	m_dxrGlobalRootSignature = std::make_unique<DX12Utils::RootSignature>("dxrGlobal");
	m_dxrGlobalRootSignature->addSRV("AccelerationStructure", 0);
	m_dxrGlobalRootSignature->addCBV("SceneCBuffer", 0);
	m_dxrGlobalRootSignature->addSRV("WaterData", 6, 0);

	m_dxrGlobalRootSignature->build(m_context->getDevice());
}

void DXRBase::createRayGenLocalRootSignature() {
	m_localSignatureRayGen = std::make_unique<DX12Utils::RootSignature>("RayGenLocal");
	m_localSignatureRayGen->addDescriptorTable("OutputUAV", D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0);
	m_localSignatureRayGen->addDescriptorTable("OutputBloomUAV", D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1);
	m_localSignatureRayGen->addDescriptorTable("gbufferInputTextures", D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 0U, DX12GBufferRenderer::NUM_GBUFFERS + 1);
	m_localSignatureRayGen->addDescriptorTable("gbufferInputTextures", D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10 + DX12GBufferRenderer::NUM_GBUFFERS + 1, 0U, 3U);
	m_localSignatureRayGen->addDescriptorTable("sys_brdfLUT", D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5);
	m_localSignatureRayGen->addCBV("DecalCBuffer", 2, 0);
	m_localSignatureRayGen->addStaticSampler();

	m_localSignatureRayGen->build(m_context->getDevice(), D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
}

void DXRBase::createHitGroupLocalRootSignature() {
	m_localSignatureHitGroup_mesh = std::make_unique<DX12Utils::RootSignature>("HitGroupLocal");
	m_localSignatureHitGroup_mesh->addDescriptorTable("sys_brdfLUT", D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5);
	m_localSignatureHitGroup_mesh->addSRV("VertexBuffer", 1, 0);
	m_localSignatureHitGroup_mesh->addSRV("IndexBuffer", 1, 1);
	m_localSignatureHitGroup_mesh->addCBV("MeshCBuffer", 1, 0);
	m_localSignatureHitGroup_mesh->addDescriptorTable("Textures", D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 3); // Textures (t0, t1, t2)
	m_localSignatureHitGroup_mesh->addStaticSampler();
	m_localSignatureHitGroup_mesh->build(m_context->getDevice(), D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

	/*==========Metaballs=========*/
	m_localSignatureHitGroup_metaball = std::make_unique<DX12Utils::RootSignature>("HitGroupLocal2");
	m_localSignatureHitGroup_metaball->addDescriptorTable("sys_brdfLUT", D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5);
	m_localSignatureHitGroup_metaball->addSRV("MetaballPositions", 1, 2);
	m_localSignatureHitGroup_metaball->addCBV("MeshCBuffer", 1, 0);
	m_localSignatureHitGroup_metaball->addStaticSampler();
	m_localSignatureHitGroup_metaball->build(m_context->getDevice(), D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
}

void DXRBase::createMissLocalRootSignature() {
	m_localSignatureMiss = std::make_unique<DX12Utils::RootSignature>("MissLocal");
	//m_localSignatureMiss->addDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3); // Skybox
	//m_localSignatureMiss->addStaticSampler();

	m_localSignatureMiss->build(m_context->getDevice(), D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
}

void DXRBase::initMetaballBuffers() {
	m_metaballPositions_srv.reserve(DX12API::NUM_SWAP_BUFFERS);
	for (size_t i = 0; i < DX12API::NUM_SWAP_BUFFERS; i++) {
		m_metaballPositions_srv.emplace_back(DX12Utils::CreateBuffer(m_context->getDevice(), MAX_NUM_METABALLS * sizeof(glm::vec3), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, DX12Utils::sUploadHeapProperties));
		m_metaballPositions_srv.back()->SetName(L"Metaball Positions");
	}
}

void DXRBase::initDecals(D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle, D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle) {
	auto& resMan = Application::getInstance()->getResourceManager();
	resMan.loadTexture(m_decalTexPaths[0]);
	resMan.loadTexture(m_decalTexPaths[1]);
	resMan.loadTexture(m_decalTexPaths[2]);

	m_decalTexGPUHandles = *gpuHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptors[3];
	srcDescriptors[0] = static_cast<DX12Texture*>(&resMan.getTexture(m_decalTexPaths[0]))->getSrvCDH();
	srcDescriptors[1] = static_cast<DX12Texture*>(&resMan.getTexture(m_decalTexPaths[1]))->getSrvCDH();
	srcDescriptors[2] = static_cast<DX12Texture*>(&resMan.getTexture(m_decalTexPaths[2]))->getSrvCDH();

	UINT dstRangeSizes[] = {3};
	UINT srcRangeSizes[] = {1, 1, 1};
	m_context->getDevice()->CopyDescriptors(1, cpuHandle, dstRangeSizes, 3, srcDescriptors, srcRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cpuHandle->ptr += m_heapIncr * 3;
	gpuHandle->ptr += m_heapIncr * 3;
	m_usedDescriptors += 3;
}

void DXRBase::addMetaballGroupAABB(int index) {
	std::vector<ID3D12Resource1*> & aabbRes = m_aabb_desc_resources[index];

	aabbRes.reserve(DX12API::NUM_GPU_BUFFERS);
	for (size_t i = 0; i < DX12API::NUM_GPU_BUFFERS; i++) {
		aabbRes.emplace_back(DX12Utils::CreateBuffer(m_context->getDevice(), sizeof(D3D12_RAYTRACING_AABB), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, DX12Utils::sUploadHeapProperties));
		aabbRes.back()->SetName(L"Metaball Group AABB");
	}
}

void DXRBase::createEmptyLocalRootSignature() {
	m_localSignatureEmpty = std::make_unique<DX12Utils::RootSignature>("EmptyLocal");
	m_localSignatureEmpty->build(m_context->getDevice(), D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
}
