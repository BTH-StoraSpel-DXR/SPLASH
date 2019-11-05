#pragma once

#include "../DX12API.h"
#include "DXRUtils.h"
#include "../DX12Utils.h"
#include "Sail/api/Renderer.h"
#include "../shader/DX12ConstantBuffer.h"
#include "../shader/DX12StructuredBuffer.h"
#include "API/DX12/resources/DX12RenderableTexture.h"

// Include defines shared with dxr shaders
#include "Sail/../../SPLASH/res/shaders/dxr/Common_hlsl_cpp.hlsl"

class DXRBase : public IEventListener {
public:
	// TODO: somehow allow this to change from different DXRBase instances
	struct RayPayload {
		glm::vec4 color;
		UINT recursionDepth;
		int hit;
	};

	struct Metaball {
		glm::vec3 pos;
		float distToCamera;
	};

	struct Cluster {
		void Add(const Metaball* ball, float dist2) {
			if (dist2 > m_mostDistant_dist || m_metaballs.empty()) {
				//m_mostDistant = ball;
				m_mostDistant_dist = dist2;
			}

			m_metaballs.push_back(ball);
		}

		//const Metaball* m_mostDistant;
		float m_mostDistant_dist;
		glm::vec3 m_center;
		std::vector<const Metaball*> m_metaballs;
	};

	DXRBase(const std::string& shaderFilename, DX12RenderableTexture** inputs);
	~DXRBase();

	void setGBufferInputs(DX12RenderableTexture** inputs);

	void updateAccelerationStructures(const std::vector<Renderer::RenderCommand>& sceneGeometry, ID3D12GraphicsCommandList4* cmdList);
	void updateSceneData(Camera& cam, LightSetup& lights, const std::vector<Metaball>& metaballs, const D3D12_RAYTRACING_AABB& m_next_metaball_aabb, const glm::vec3& mapSize, const glm::vec3& mapStart);
	void updateDecalData(DXRShaderCommon::DecalData* decals, size_t size);
	void addWaterAtWorldPosition(const glm::vec3& position);
	void updateWaterData();
	void dispatch(DX12RenderableTexture* outputTexture, ID3D12GraphicsCommandList4* cmdList);

	void resetWater();
	void reloadShaders();

	virtual bool onEvent(Event& event) override;

private:
	struct AccelerationStructureBuffers {
		wComPtr<ID3D12Resource1> scratch = nullptr;
		wComPtr<ID3D12Resource1> result = nullptr;
		wComPtr<ID3D12Resource1> instanceDesc = nullptr;    // Used only for top-level AS
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
	struct InstanceList {
		AccelerationStructureBuffers blas;
		std::vector<glm::mat3x4> instanceTransforms;
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
	void updateMetaballpositions(const std::vector<Metaball>& metaballs, const D3D12_RAYTRACING_AABB& m_next_metaball_aabb);
	void metaballClustering(const std::vector<Metaball>& metaballs, const D3D12_RAYTRACING_AABB& m_next_metaball_aabb);

	void initDecals(D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle, D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle);

private:
	DX12API* m_context;

	DX12RenderableTexture** m_gbufferInputTextures;
	DX12Texture** m_decalInputTextures;

	std::string m_shaderFilename;

	std::unique_ptr<ShaderComponent::DX12ConstantBuffer> m_sceneCB;
	std::unique_ptr<ShaderComponent::DX12ConstantBuffer> m_meshCB;
	std::unique_ptr<ShaderComponent::DX12ConstantBuffer> m_decalCB;

	std::vector<std::unordered_map<Mesh*, InstanceList>> m_bottomBuffers;
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
	std::string m_decalTexPaths[3];

	wComPtr<ID3D12DescriptorHeap> m_rtDescriptorHeap = {};
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtHeapCPUHandle[2];
	D3D12_GPU_DESCRIPTOR_HANDLE m_rtHeapGPUHandle[2];
	D3D12_GPU_DESCRIPTOR_HANDLE m_rtOutputTextureUavGPUHandles[2];
	D3D12_GPU_DESCRIPTOR_HANDLE m_rtBrdfLUTGPUHandle;
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> m_gbufferStartGPUHandles;
	D3D12_GPU_DESCRIPTOR_HANDLE m_decalTexGPUHandles;
	UINT m_heapIncr;
	UINT m_usedDescriptors;

	std::vector<MeshHandles> m_rtMeshHandles[2];
	// Metaballs
	std::vector<ID3D12Resource1*> m_metaballPositions_srv;
	UINT m_metaballsToRender;
	// Decals
	UINT m_decalsToRender;

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
	std::vector<ID3D12Resource1*> m_aabb_desc_resource; // m_aabb_desc uploaded to GPU
	Cluster* m_clusters = nullptr;
	const int m_N_CLUSTERS = 8;

	// Water voxel grid stuff
	std::unique_ptr<ShaderComponent::DX12StructuredBuffer> m_waterStructuredBuffer;
	std::unordered_map<unsigned int, unsigned int> m_waterDeltas; // Changed water voxels over the last 2 frames
	unsigned int m_waterDataCPU[WATER_ARR_SIZE];
	bool m_waterChanged;

};