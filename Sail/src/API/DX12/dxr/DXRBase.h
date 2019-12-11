#pragma once

#include "../DX12API.h"
#include "DXRUtils.h"
#include "../DX12Utils.h"
#include "Sail/api/Renderer.h"
#include "../shader/DX12ConstantBuffer.h"
#include "../shader/DX12StructuredBuffer.h"
#include "API/DX12/resources/DX12RenderableTexture.h"

#include <bitset>

// Include defines shared with dxr shaders
#include "Sail/../../SPLASH/res/shaders/dxr/Common_hlsl_cpp.hlsl"

class DXRBase final : public EventReceiver {
public:
	struct Metaball {
		glm::vec3 pos;
		float distToCamera;
	};
	struct BounceOutput {
		std::unique_ptr<DX12RenderableTexture> albedo; // RGB
		std::unique_ptr<DX12RenderableTexture> normal; // RGB
		std::unique_ptr<DX12RenderableTexture> metalnessRoughnessAO; // RGB
		std::unique_ptr<DX12RenderableTexture> shadows; // RG - first and second bounce shadows
		std::unique_ptr<DX12RenderableTexture> positionsOne; // RGB - first bounce world positions
		std::unique_ptr<DX12RenderableTexture> positionsTwo; // RGB - second bounce world positions
	};

	struct MetaballGroup {
		int index;
		int gpuGroupStartOffset; //offset in gpu structured buffer for metaball positions
		std::vector<DXRBase::Metaball> balls;
		float averageDistToCamera;
		D3D12_RAYTRACING_AABB aabb;
	};

	DXRBase(const std::string& shaderFilename, DX12RenderableTexture** inputs);
	~DXRBase();

	void setGBufferInputs(DX12RenderableTexture** inputs);

	void updateAccelerationStructures(const std::vector<Renderer::RenderCommand>& sceneGeometry, const std::vector<DXRBase::MetaballGroup*>& metaballGroups, ID3D12GraphicsCommandList4* cmdList);

	void updateSceneData(Camera* cam, LightSetup* lights, const std::vector<DXRBase::MetaballGroup*>& metaballGroups, const std::vector<glm::vec3>& teamColors, unsigned int numShadowTextures);
	void addWaterAtWorldPosition(const glm::vec3& position);
	unsigned int removeWaterAtWorldPosition(const glm::vec3& position, const glm::ivec3& posOffset, const glm::ivec3& negOffset);
	bool checkWaterAtWorldPosition(const glm::vec3& position);
	// THIS WAS IMPLEMENTED SPECIFICALLY FOR CLEANING STATE!
	std::pair<bool, glm::vec3> getNearestWaterPosition(const glm::vec3& position, const glm::vec3& maxOffset);
	void updateWaterData();
	void dispatch(BounceOutput& output, DX12RenderableTexture* outputBloomTexture, DX12RenderableTexture* shadowsLastFrameInput, ID3D12GraphicsCommandList4* cmdList);

	void simulateWater(float dt);
	void rebuildWater();
	ShaderComponent::DX12StructuredBuffer* getWaterVoxelSBuffer();

	void resetWater();
	void reloadShaders();

	virtual bool onEvent(const Event& event) override;

private:
	struct AccelerationStructureBuffers {
		wComPtr<ID3D12Resource> scratch = nullptr;
		wComPtr<ID3D12Resource> result = nullptr;
		wComPtr<ID3D12Resource> instanceDesc = nullptr;    // Used only for top-level AS
		bool allowUpdate = false;
		void release() {
			if (scratch) {
				scratch->Release();
				scratch = nullptr;
			}
			if (result) {
				result->Release();
				result = nullptr;
			}
			if (instanceDesc) {
				instanceDesc->Release();
				instanceDesc = nullptr;
			}
		}
	};
	
	struct PerInstance {
		glm::mat3x4 transform;
		char teamColorIndex;
		bool castShadows;
	};

	struct InstanceList {
		AccelerationStructureBuffers blas;
		std::vector<PerInstance> instanceList;
	};

private:
	// Acceleration structures
	void createTLAS(unsigned int numInstanceDescriptors, ID3D12GraphicsCommandList4* cmdList);
	void createBLAS(const Renderer::RenderCommand& renderCommand, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags, ID3D12GraphicsCommandList4* cmdList, AccelerationStructureBuffers* sourceBufferForUpdate = nullptr);

	void updateDescriptorHeap(ID3D12GraphicsCommandList4* cmdList);
	void updateShaderTables();
	void createInitialShaderResources(bool remake = false);
	void createRaytracingPSO();

	// Root signature creation
	// TODO: create them dynamically after parsing the shader source (like ShaderPipeline does)
	void createDXRGlobalRootSignature();
	void createRayGenLocalRootSignature();
	void createHitGroupLocalRootSignature();
	void createMissLocalRootSignature();
	void createEmptyLocalRootSignature();

	void initMetaballBuffers();
	void updateMetaballpositions(const std::vector<DXRBase::MetaballGroup*>& metaballGroups);

	void addMetaballGroupAABB(int index);

private:
	DX12API* m_context;

	DX12RenderableTexture** m_gbufferInputTextures;

	std::string m_shaderFilename;

	std::unique_ptr<ShaderComponent::DX12ConstantBuffer> m_sceneCB;
	std::unique_ptr<ShaderComponent::DX12ConstantBuffer> m_meshCB;

	std::vector<std::unordered_map<Mesh*, InstanceList>> m_bottomBuffers;
	std::vector<std::unordered_map<int, InstanceList>> m_bottomBuffers_Metaballs;

	std::vector<AccelerationStructureBuffers> m_DXR_TopBuffer;

	wComPtr<ID3D12StateObject> m_rtPipelineState;

	std::vector<DXRUtils::ShaderTableData> m_rayGenShaderTable;
	std::vector<DXRUtils::ShaderTableData> m_missShaderTable;
	std::vector<DXRUtils::ShaderTableData> m_hitGroupShaderTable;

	struct MeshHandles {
		D3D12_GPU_VIRTUAL_ADDRESS vertexBufferHandle;
		D3D12_GPU_VIRTUAL_ADDRESS indexBufferHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE textureHandles[3];
	};

	std::string m_brdfLUTPath;

	wComPtr<ID3D12DescriptorHeap> m_rtDescriptorHeap = {};
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtHeapCPUHandle[2];
	D3D12_GPU_DESCRIPTOR_HANDLE m_rtHeapGPUHandle[2];

	// DXR shader outputs
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtOutputAlbedoUavCPUHandles[2];
	D3D12_GPU_DESCRIPTOR_HANDLE m_rtOutputAlbedoUavGPUHandles[2];
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtOutputNormalsUavCPUHandles[2];
	D3D12_GPU_DESCRIPTOR_HANDLE m_rtOutputNormalsUavGPUHandles[2];
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtOutputMetalnessRoughnessAoUavCPUHandles[2];
	D3D12_GPU_DESCRIPTOR_HANDLE m_rtOutputMetalnessRoughnessAoUavGPUHandles[2];
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtOutputShadowsUavCPUHandles[2];
	D3D12_GPU_DESCRIPTOR_HANDLE m_rtOutputShadowsUavGPUHandles[2];
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtOutputPositionsOneUavCPUHandles[2];
	D3D12_GPU_DESCRIPTOR_HANDLE m_rtOutputPositionsOneUavGPUHandles[2];
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtOutputPositionsTwoUavCPUHandles[2];
	D3D12_GPU_DESCRIPTOR_HANDLE m_rtOutputPositionsTwoUavGPUHandles[2];
	// DXR shader inputs
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtInputShadowsLastFrameUavCPUHandles[2];
	D3D12_GPU_DESCRIPTOR_HANDLE m_rtInputShadowsLastFrameUavGPUHandles[2];

	D3D12_CPU_DESCRIPTOR_HANDLE m_rtOutputBloomTextureUavCPUHandles[2];
	D3D12_GPU_DESCRIPTOR_HANDLE m_rtOutputBloomTextureUavGPUHandles[2];

	D3D12_GPU_DESCRIPTOR_HANDLE m_rtBrdfLUTGPUHandle;
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> m_gbufferStartUAVGPUHandles;
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> m_gbufferStartSRVGPUHandles;
	UINT m_heapIncr;
	UINT m_usedDescriptors;

	std::vector<MeshHandles> m_rtMeshHandles[2];
	// Metaballs
	std::vector<ID3D12Resource*> m_metaballPositions_srv;

	const WCHAR* m_rayGenName = L"rayGen";
	const WCHAR* m_closestHitName = L"closestHitTriangle";
	const WCHAR* m_closestProceduralPrimitive = L"closestHitProcedural";
	const WCHAR* m_intersectionProceduralPrimitive = L"IntersectionShader";
	const WCHAR* m_missName = L"miss";
	const WCHAR* m_hitGroupTriangleName = L"hitGroupTriangle";
	const WCHAR* m_hitGroupMetaBallName = L"hitGroupMetaBall";
	const WCHAR* m_shadowMissName = L"shadowMiss";

	std::unique_ptr<DX12Utils::RootSignature> m_dxrGlobalRootSignature;
	std::unique_ptr<DX12Utils::RootSignature> m_localSignatureRayGen;
	std::unique_ptr<DX12Utils::RootSignature> m_localSignatureHitGroup_mesh;
	std::unique_ptr<DX12Utils::RootSignature> m_localSignatureHitGroup_metaball;
	std::unique_ptr<DX12Utils::RootSignature> m_localSignatureMiss;
	std::unique_ptr<DX12Utils::RootSignature> m_localSignatureEmpty;

	// Metaball Stuff
	std::map<int, std::vector<ID3D12Resource*>> m_aabb_desc_resources; // m_aabb_descs uploaded to GPU

	// Water voxel grid stuff
	std::unique_ptr<ShaderComponent::DX12StructuredBuffer> m_waterStructuredBuffer;
	std::unordered_map<unsigned int, unsigned int> m_waterDeltas; // Changed water voxels over the last 2 frames
	unsigned int* m_waterDataCPU;
	bool* m_updateWater;
	bool m_waterChanged;
	glm::vec3 m_waterArrSizes;
	unsigned int m_waterArrSize;
	glm::vec3 m_mapSize;
	glm::vec3 m_mapStart;

	// Water simluation stuff
	int m_currWaterZChunk;
	int m_maxWaterZChunk;
	int m_waterZChunkSize;

	// Temporal accumulation stuff
	unsigned int m_frameCount;
};