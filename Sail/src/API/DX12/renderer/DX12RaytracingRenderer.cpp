#include "pch.h"
#include "DX12RaytracingRenderer.h"
#include "Sail/Application.h"
#include "../DX12Utils.h"
#include "Sail/graphics/postprocessing/PostProcessPipeline.h"
#include "API/DX12/DX12VertexBuffer.h"
#include "Sail/KeyBinds.h"
#include "Sail/entities/components/MapComponent.h"

// Current goal is to make this render a fully raytraced image of all geometry (without materials) within a scene

DX12RaytracingRenderer::DX12RaytracingRenderer(DX12RenderableTexture** inputs)
	: m_dxr("Basic", inputs)
	, m_asFenceValues()
	, m_dispatchFenceValues()
{
	Application* app = Application::getInstance();
	m_context = app->getAPI<DX12API>();
	m_context->initCommand(m_commandDirect, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_commandDirect.list->SetName(L"Raytracing Renderer main DIRECT command list");
	m_context->initCommand(m_commandComputeAS, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	m_commandComputeAS.list->SetName(L"Raytracing Renderer COMPUTE AS command list");
	m_context->initCommand(m_commandComputeDispatch, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	m_commandComputeDispatch.list->SetName(L"Raytracing Renderer COMPUTE DISPATCH command list");

	m_eventHandle = CreateEvent(0, false, false, 0);

	auto windowWidth = app->getWindow()->getWindowWidth();
	auto windowHeight = app->getWindow()->getWindowHeight();
	m_outputTexture = std::unique_ptr<DX12RenderableTexture>(static_cast<DX12RenderableTexture*>(RenderableTexture::Create(windowWidth, windowHeight)));

	m_currNumDecals = 0;
	memset(m_decals, 0, sizeof(DXRShaderCommon::DecalData) * MAX_DECALS);
}

DX12RaytracingRenderer::~DX12RaytracingRenderer() {
	
}

void DX12RaytracingRenderer::present(PostProcessPipeline* postProcessPipeline, RenderableTexture* output) {
	auto frameIndex = m_context->getSwapIndex();
	auto lastFrameIndex = 1 - frameIndex;

	// There is one allocator per swap buffer
	auto& allocatorComputeAS = m_commandComputeAS.allocators[m_context->getFrameIndex()];
	auto& cmdListComputeAS = m_commandComputeAS.list;
	auto& allocatorComputeDispatch = m_commandComputeDispatch.allocators[m_context->getFrameIndex()];
	auto& cmdListComputeDispatch = m_commandComputeDispatch.list;
	auto& allocatorDirect = m_commandDirect.allocators[m_context->getFrameIndex()];
	auto& cmdListDirect = m_commandDirect.list;

	// Reset allocators and lists for this frame
	allocatorComputeAS->Reset();
	cmdListComputeAS->Reset(allocatorComputeAS.Get(), nullptr);
	allocatorComputeDispatch->Reset();
	cmdListComputeDispatch->Reset(allocatorComputeDispatch.Get(), nullptr);
	allocatorDirect->Reset();
	cmdListDirect->Reset(allocatorDirect.Get(), nullptr);

	// Wait for the G-Buffer pass to finish execution on the direct queue
	auto fenceVal = m_context->getDirectQueue()->signal();
	m_context->getMainComputeQueue()->wait(fenceVal);
	m_context->getAsyncComputeQueue()->wait(fenceVal);

	// Clear output texture
	//m_outputTexture.get()->clear({ 0.01f, 0.01f, 0.01f, 1.0f }, cmdListDirect.Get());

	std::sort(m_metaballpositions.begin(), m_metaballpositions.end(),
		[](const DXRBase::Metaball& a, const DXRBase::Metaball& b) -> const bool {
		return a.distToCamera < b.distToCamera;
	});

	for (size_t i = 0; i < MAX_NUM_METABALLS && i < m_metaballpositions.size(); i++) {
		commandQueue.emplace_back();
		RenderCommand& cmd = commandQueue.back();
		cmd.type = RENDER_COMMAND_TYPE_NON_MODEL_METABALL;
		cmd.nonModel.material = nullptr;
		cmd.transform = glm::identity<glm::mat4>();
		cmd.transform = glm::translate(cmd.transform, m_metaballpositions[i].pos);
		cmd.transform = glm::transpose(cmd.transform);
		cmd.hasUpdatedSinceLastRender.resize(m_context->getNumGPUBuffers(), false);
	}

	if (Input::WasKeyJustPressed(KeyBinds::reloadDXRShader)) {
		m_dxr.reloadShaders();
	}

	// Copy updated flag from vertex buffers to renderCommand
	// This marks all commands that should have their bottom layer updated in-place
	for (auto& renderCommand : commandQueue) {
		if (renderCommand.type == RENDER_COMMAND_TYPE_MODEL) {
			auto& vb = static_cast<DX12VertexBuffer&>(renderCommand.model.mesh->getVertexBuffer());
			if (vb.hasBeenUpdated()) {
				renderCommand.hasUpdatedSinceLastRender[frameIndex] = true;
				vb.resetHasBeenUpdated();
			}
		}
	}

	// Decrease water radius over time
	/*for (auto& r : m_waterData) {
		if (r > 0.f) {
			r -= Application::getInstance()->getDelta() * 0.01f;
		}
	}*/

	if (camera && lightSetup) {
		static auto mapSize = glm::vec3(MapComponent::xsize, 1.0f, MapComponent::ysize) * (float)MapComponent::tileSize;
		static auto mapStart = -glm::vec3(MapComponent::tileSize / 2.0f);
		m_dxr.updateSceneData(*camera, *lightSetup, m_metaballpositions, mapSize, mapStart);
	}
	m_dxr.updateDecalData(m_decals, m_currNumDecals > MAX_DECALS - 1 ? MAX_DECALS : m_currNumDecals);
	m_dxr.updateWaterData();

	m_context->getAsyncComputeQueue()->waitOnCPU(m_dispatchFenceValues[lastFrameIndex], m_eventHandle);
	//Logger::Log("ASYNC queue before updateAS waited for " + std::to_string(m_dispatchFenceValues[lastFrameIndex]));
	m_dxr.updateAccelerationStructures(commandQueue, cmdListComputeAS.Get());
	// AS has now been updated this frame, reset flag
	for (auto& renderCommand : commandQueue) {
		renderCommand.hasUpdatedSinceLastRender[frameIndex] = false;
	}
	// Start executing AS update on async queue
	cmdListComputeAS->Close();
	m_context->executeCommandLists({ cmdListComputeAS.Get() }, D3D12_COMMAND_LIST_TYPE_COMPUTE, true);
	m_asFenceValues[frameIndex] = m_context->getAsyncComputeQueue()->signal();
	// Logger::Log("ASYNC queue after updateAS signaled " + std::to_string(m_asFenceValues[frameIndex]));

	static bool firstFrame = true;
	if (!firstFrame) {
		m_context->getMainComputeQueue()->wait(m_asFenceValues[lastFrameIndex]);
		// Logger::Log("Main queue before dispatch waited for " + std::to_string(m_asFenceValues[lastFrameIndex]));
		m_dxr.dispatch(m_outputTexture.get(), cmdListComputeDispatch.Get());
	}
	firstFrame = false;

	// TODO: move this to a graphics queue when current cmdList is executed on the compute queue

	RenderableTexture* renderOutput = m_outputTexture.get();
	if (postProcessPipeline) {
		// Run post processing
		RenderableTexture* ppOutput = postProcessPipeline->run(m_outputTexture.get(), cmdListComputeDispatch.Get());
		if (ppOutput) {
			renderOutput = ppOutput;
		}
	}
	DX12RenderableTexture* dxRenderOutput = static_cast<DX12RenderableTexture*>(renderOutput);
	dxRenderOutput->transitionStateTo(cmdListComputeDispatch.Get(), D3D12_RESOURCE_STATE_COMMON);

	// Execute compute command list
	cmdListComputeDispatch->Close();
	m_context->executeCommandLists({ cmdListComputeDispatch.Get() }, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	// Place a signal to syncronize copying the raytracing output to the backbuffer when it is available
	m_dispatchFenceValues[frameIndex] = m_context->getMainComputeQueue()->signal();
	// Logger::Log("Main queue after dispatch signaled " + std::to_string(m_dispatchFenceValues[frameIndex]));

	// Copy post processing output to back buffer
	dxRenderOutput->transitionStateTo(cmdListDirect.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE);
	auto* renderTarget = m_context->getCurrentRenderTargetResource();
	DX12Utils::SetResourceTransitionBarrier(cmdListDirect.Get(), renderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
	cmdListDirect->CopyResource(renderTarget, dxRenderOutput->getResource());
	// Lastly - transition back buffer to present
	DX12Utils::SetResourceTransitionBarrier(cmdListDirect.Get(), renderTarget, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);

	// Wait for compute to finish
	m_context->getDirectQueue()->wait(m_dispatchFenceValues[frameIndex]);
	// Execute direct command list
	cmdListDirect->Close();
	m_context->executeCommandLists({ cmdListDirect.Get() }, D3D12_COMMAND_LIST_TYPE_DIRECT);
}

void DX12RaytracingRenderer::begin(Camera* camera) {
	Renderer::begin(camera);
	m_metaballpositions.clear();
}

bool DX12RaytracingRenderer::onEvent(Event& event) {
	EventHandler::dispatch<WindowResizeEvent>(event, SAIL_BIND_EVENT(&DX12RaytracingRenderer::onResize));

	// Pass along events
	m_dxr.onEvent(event);
	return true;
}

void DX12RaytracingRenderer::submit(Mesh* mesh, const glm::mat4& modelMatrix, RenderFlag flags) {
	RenderCommand cmd;
	cmd.type = RENDER_COMMAND_TYPE_MODEL;
	cmd.model.mesh = mesh;
	cmd.transform = glm::transpose(modelMatrix);
	cmd.flags = flags;
	// Resize to match numSwapBuffers (specific to dx12)
	cmd.hasUpdatedSinceLastRender.resize(m_context->getNumGPUBuffers(), false);
	commandQueue.push_back(cmd);
}

void DX12RaytracingRenderer::submitNonMesh(RenderCommandType type, Material* material, const glm::mat4& modelMatrix, RenderFlag flags) {
	assert(type != RenderCommandType::RENDER_COMMAND_TYPE_MODEL);

	if (type == RenderCommandType::RENDER_COMMAND_TYPE_NON_MODEL_METABALL) {
		DXRBase::Metaball ball;
		ball.pos = glm::vec3(modelMatrix[3].x, modelMatrix[3].y, modelMatrix[3].z);
		ball.distToCamera = glm::length(ball.pos - camera->getPosition());
		m_metaballpositions.emplace_back(ball);
	}
}

void DX12RaytracingRenderer::submitDecal(const glm::vec3& pos, const glm::mat3& rot, const glm::vec3& halfSize) {
	DXRShaderCommon::DecalData decalData;
	decalData.position = pos;
	decalData.rot = rot;
	decalData.halfSize = halfSize;
	m_decals[m_currNumDecals % MAX_DECALS] = decalData;
	m_currNumDecals++;
}

void DX12RaytracingRenderer::submitWaterPoint(const glm::vec3& pos) {
	m_dxr.addWaterAtWorldPosition(pos);
}

void DX12RaytracingRenderer::setGBufferInputs(DX12RenderableTexture** inputs) {
	m_dxr.setGBufferInputs(inputs);
}

bool DX12RaytracingRenderer::onResize(WindowResizeEvent& event) {
	m_outputTexture->resize(event.getWidth(), event.getHeight());
	return true;
}