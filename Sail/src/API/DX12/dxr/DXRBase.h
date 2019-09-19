#pragma once

#include "../DX12API.h"
#include "DXRUtils.h"
#include "../DX12Utils.h"
#include "Sail/api/Renderer.h"
#include "../shader/DX12ConstantBuffer.h"
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

	DXRBase(const std::string& shaderFilename);
	~DXRBase();

	void updateAccelerationStructures(const std::vector<Renderer::RenderCommand>& sceneGeometry, ID3D12GraphicsCommandList4* cmdList);
	void updateCamera(Camera& cam);
	void dispatch(DX12RenderableTexture* outputTexture, ID3D12GraphicsCommandList4* cmdList);

	virtual bool onEvent(Event& event) override;

private:
	// Acceleration structures
	void createTLAS(const std::vector<Renderer::RenderCommand>& sceneGeometry, ID3D12GraphicsCommandList4* cmdList);
	void createBLAS(const std::vector<Renderer::RenderCommand>& sceneGeometry, ID3D12GraphicsCommandList4* cmdList);

	// Other DXR requirements
	void createShaderTables(const std::vector<Renderer::RenderCommand>& sceneGeometry);
	void createShaderResources(bool remake = false);
	void createRaytracingPSO();

	// Root signature creation
	// TODO: create them dynamically after parsing the shader source (like ShaderPipeline does)
	void createDXRGlobalRootSignature();
	void createRayGenLocalRootSignature();
	void createHitGroupLocalRootSignature();
	void createMissLocalRootSignature();

private:
	DX12API* m_context;
	std::string m_shaderFilename;

	//union AlignedSceneConstantBuffer { 	// TODO: Use this instead of SceneConstantBuffer
	//	SceneConstantBuffer* constants;
	//	uint8_t alignmentPadding[D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT];
	//};
	//AlignedSceneConstantBuffer* m_mappedSceneCBData; // TODO: Fix memory leak
	//SceneConstantBuffer* m_sceneCBData; // TODO: Fix memory leak
	//RayGenSettings m_rayGenCBData;

	std::vector<std::unique_ptr<ShaderComponent::DX12ConstantBuffer>> m_cameraCB;
	std::vector<std::unique_ptr<ShaderComponent::DX12ConstantBuffer>> m_meshCB;

	struct AccelerationStructureBuffers {
		wComPtr<ID3D12Resource1> scratch = nullptr;
		wComPtr<ID3D12Resource1> result = nullptr;
		wComPtr<ID3D12Resource1> instanceDesc = nullptr;    // Used only for top-level AS
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
	std::vector<std::vector<AccelerationStructureBuffers>> m_DXR_BottomBuffers;
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

	wComPtr<ID3D12DescriptorHeap> m_rtDescriptorHeap = {};
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtHeapCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_rtHeapGPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_rtOutputTextureUavGPUHandle;
	UINT m_heapIncr;

	std::vector<MeshHandles> m_rtMeshHandles;

	const WCHAR* m_rayGenName = L"rayGen";
	const WCHAR* m_closestHitName = L"closestHit";
	const WCHAR* m_missName = L"miss";
	const WCHAR* m_hitGroupName = L"HitGroup";

	std::unique_ptr<DX12Utils::RootSignature> m_dxrGlobalRootSignature;
	std::unique_ptr<DX12Utils::RootSignature> m_localSignatureRayGen;
	std::unique_ptr<DX12Utils::RootSignature> m_localSignatureHitGroup;
	std::unique_ptr<DX12Utils::RootSignature> m_localSignatureMiss;


};