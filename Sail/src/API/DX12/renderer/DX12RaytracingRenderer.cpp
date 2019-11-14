#include "pch.h"
#include "DX12RaytracingRenderer.h"
#include "Sail/Application.h"
#include "../DX12Utils.h"
#include "Sail/graphics/postprocessing/PostProcessPipeline.h"
#include "API/DX12/DX12VertexBuffer.h"
#include "Sail/KeyBinds.h"
#include "Sail/entities/systems/Gameplay/LevelSystem/LevelSystem.h"
#include "Sail/events/EventDispatcher.h"

// Current goal is to make this render a fully raytraced image of all geometry (without materials) within a scene

DX12RaytracingRenderer::DX12RaytracingRenderer(DX12RenderableTexture** inputs)
	: m_dxr("Basic", inputs) {

	EventDispatcher::Instance().subscribe(Event::Type::WINDOW_RESIZE, this);

	Application* app = Application::getInstance();
	m_context = app->getAPI<DX12API>();
	m_context->initCommand(m_commandDirect, D3D12_COMMAND_LIST_TYPE_DIRECT, L"Raytracing Renderer DIRECT command list or allocator");
	m_context->initCommand(m_commandCompute, D3D12_COMMAND_LIST_TYPE_COMPUTE, L"Raytracing Renderer main COMPUTE command list or allocator");

	auto windowWidth = app->getWindow()->getWindowWidth();
	auto windowHeight = app->getWindow()->getWindowHeight();
	m_outputTexture = std::unique_ptr<DX12RenderableTexture>(static_cast<DX12RenderableTexture*>(RenderableTexture::Create(windowWidth, windowHeight, "Raytracing renderer output texture", Texture::R16G16B16A16_FLOAT)));
	m_outputBloomTexture = std::unique_ptr<DX12RenderableTexture>(static_cast<DX12RenderableTexture*>(RenderableTexture::Create(windowWidth, windowHeight, "Raytracing renderer bloom output texture", Texture::R16G16B16A16_FLOAT)));

	m_currNumDecals = 0;
	memset(m_decals, 0, sizeof(DXRShaderCommon::DecalData) * MAX_DECALS);
}

DX12RaytracingRenderer::~DX12RaytracingRenderer() {
	EventDispatcher::Instance().unsubscribe(Event::Type::WINDOW_RESIZE, this);
}

void DX12RaytracingRenderer::present(PostProcessPipeline* postProcessPipeline, RenderableTexture* output) {
	auto frameIndex = m_context->getSwapIndex();

	if (postProcessPipeline) {
		// Make sure output textures are in a higher precision format to accomodate values > 1
		m_outputTexture->changeFormat(Texture::R16G16B16A16_FLOAT);
		m_outputBloomTexture->changeFormat(Texture::R16G16B16A16_FLOAT);
	} else {
		// Make sure output textures are in a format that can be directly copied to the back buffer
		// Please note that no tone mapping is applied when post process is turned off.
		m_outputTexture->changeFormat(Texture::R8G8B8A8);
		m_outputBloomTexture->changeFormat(Texture::R8G8B8A8);
	}

	// There is one allocator per swap buffer
	auto& allocatorCompute = m_commandCompute.allocators[m_context->getFrameIndex()];
	auto& allocatorDirect = m_commandDirect.allocators[m_context->getFrameIndex()];
	auto& cmdListCompute = m_commandCompute.list;
	auto& cmdListDirect = m_commandDirect.list;

	// Reset allocators and lists for this frame
	allocatorCompute->Reset();
	allocatorDirect->Reset();
	cmdListCompute->Reset(allocatorCompute.Get(), nullptr);
	cmdListDirect->Reset(allocatorDirect.Get(), nullptr);

	// Wait for the G-Buffer pass to finish execution on the direct queue
	auto fenceVal = m_context->getDirectQueue()->signal();
	m_context->getComputeQueue()->wait(fenceVal);

	std::sort(m_metaballs.begin(), m_metaballs.end(),
		[](const DXRBase::Metaball& a, const DXRBase::Metaball& b) -> const bool
		{
			return a.distToCamera < b.distToCamera;
		});

	if (m_metaballs.size() > 0) {
		commandQueue.emplace_back();
		RenderCommand& cmd = commandQueue.back();
		cmd.type = RENDER_COMMAND_TYPE_NON_MODEL_METABALL;
		cmd.flags = Renderer::MESH_DYNAMIC;
		cmd.nonModel.material = nullptr;
		cmd.transform = glm::identity<glm::mat4>();
		cmd.transform = glm::transpose(cmd.transform);
		cmd.hasUpdatedSinceLastRender.resize(m_context->getNumGPUBuffers(), true);

		//Calculate the needed size of m_next_metaball_aabb.
		glm::vec3& pos = m_metaballs.front().pos;
		m_nextMetaballAabb.MaxX = pos.x + METABALL_RADIUS;
		m_nextMetaballAabb.MaxY = pos.y + METABALL_RADIUS;
		m_nextMetaballAabb.MaxZ = pos.z + METABALL_RADIUS;
		m_nextMetaballAabb.MinX = pos.x - METABALL_RADIUS;
		m_nextMetaballAabb.MinY = pos.y - METABALL_RADIUS;
		m_nextMetaballAabb.MinZ = pos.z - METABALL_RADIUS;
		for (size_t i = 1; i < m_metaballs.size() && i < MAX_NUM_METABALLS; i++) {
			glm::vec3& pos = m_metaballs[i].pos;

			if (m_nextMetaballAabb.MaxX < pos.x + METABALL_RADIUS) {
				m_nextMetaballAabb.MaxX = pos.x + METABALL_RADIUS;
			}
			if (m_nextMetaballAabb.MaxY < pos.y + METABALL_RADIUS) {
				m_nextMetaballAabb.MaxY = pos.y + METABALL_RADIUS;
			}
			if (m_nextMetaballAabb.MaxZ < pos.z + METABALL_RADIUS) {
				m_nextMetaballAabb.MaxZ = pos.z + METABALL_RADIUS;
			}

			if (m_nextMetaballAabb.MinX > pos.x - METABALL_RADIUS) {
				m_nextMetaballAabb.MinX = pos.x - METABALL_RADIUS;
			}
			if (m_nextMetaballAabb.MinY > pos.y - METABALL_RADIUS) {
				m_nextMetaballAabb.MinY = pos.y - METABALL_RADIUS;
			}
			if (m_nextMetaballAabb.MinZ > pos.z - METABALL_RADIUS) {
				m_nextMetaballAabb.MinZ = pos.z - METABALL_RADIUS;
			}
		}
	}

	if (Input::WasKeyJustPressed(KeyBinds::RELOAD_DXR_SHADER)) {
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

	if (camera && lightSetup) {
		auto& mapSettings = Application::getInstance()->getSettings().gameSettingsDynamic["map"];

		auto mapSize = glm::vec3(mapSettings["sizeX"].value, 1.0f, mapSettings["sizeY"].value) * (float)mapSettings["tileSize"].value;
		auto mapStart = -glm::vec3((float)mapSettings["tileSize"].value / 2.0f);
		m_dxr.updateSceneData(*camera, *lightSetup, m_metaballs, m_nextMetaballAabb, mapSize, mapStart, teamColors, (postProcessPipeline) ? false : true);
	}

	m_dxr.updateDecalData(m_decals, m_currNumDecals > MAX_DECALS - 1 ? MAX_DECALS : m_currNumDecals);
	m_dxr.updateWaterData();
	m_dxr.updateAccelerationStructures(commandQueue, cmdListCompute.Get());
	m_dxr.dispatch(m_outputTexture.get(), m_outputBloomTexture.get(), cmdListCompute.Get());
	// AS has now been updated this frame, reset flag
	for (auto& renderCommand : commandQueue) {
		renderCommand.hasUpdatedSinceLastRender[frameIndex] = false;
	}

	// TODO: move this to a graphics queue when current cmdList is executed on the compute queue

	RenderableTexture* renderOutput = m_outputTexture.get();
	if (postProcessPipeline) {
		// Run post processing
		postProcessPipeline->setBloomInput(m_outputBloomTexture.get());
		RenderableTexture* ppOutput = postProcessPipeline->run(m_outputTexture.get(), cmdListCompute.Get());
		if (ppOutput) {
			renderOutput = ppOutput;
		}
	}
	DX12RenderableTexture* dxRenderOutput = static_cast<DX12RenderableTexture*>(renderOutput);
	dxRenderOutput->transitionStateTo(cmdListCompute.Get(), D3D12_RESOURCE_STATE_COMMON);

	// Execute compute command list
	cmdListCompute->Close();
	m_context->executeCommandLists({ cmdListCompute.Get() }, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	// Place a signal to syncronize copying the raytracing output to the backbuffer when it is available
	fenceVal = m_context->getComputeQueue()->signal();

	// Copy post processing output to back buffer
	dxRenderOutput->transitionStateTo(cmdListDirect.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE);
	auto* renderTarget = m_context->getCurrentRenderTargetResource();
	DX12Utils::SetResourceTransitionBarrier(cmdListDirect.Get(), renderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
	cmdListDirect->CopyResource(renderTarget, dxRenderOutput->getResource());
	// Lastly - transition back buffer to present
	DX12Utils::SetResourceTransitionBarrier(cmdListDirect.Get(), renderTarget, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);

	// Wait for compute to finish
	m_context->getDirectQueue()->wait(fenceVal);
	// Execute direct command list
	cmdListDirect->Close();
	m_context->executeCommandLists({ cmdListDirect.Get() }, D3D12_COMMAND_LIST_TYPE_DIRECT);
}

void DX12RaytracingRenderer::begin(Camera* camera) {
	Renderer::begin(camera);
	m_metaballs.clear();
}

bool DX12RaytracingRenderer::onEvent(const Event& event) {
	switch (event.type) {
	case Event::Type::WINDOW_RESIZE: onResize((const WindowResizeEvent&)event); break;
	default: break;
	}

	// Pass along events
	m_dxr.onEvent(event);
	return true;
}

void DX12RaytracingRenderer::submit(Mesh* mesh, const glm::mat4& modelMatrix, RenderFlag flags, int teamColorID) {
	RenderCommand cmd;
	cmd.type = RENDER_COMMAND_TYPE_MODEL;
	cmd.model.mesh = mesh;
	cmd.transform = glm::transpose(modelMatrix);
	cmd.flags = flags;
	cmd.teamColorID = teamColorID;
	// Resize to match numSwapBuffers (specific to dx12)
	cmd.hasUpdatedSinceLastRender.resize(m_context->getNumGPUBuffers(), false);
	commandQueue.push_back(cmd);
}

void DX12RaytracingRenderer::submitMetaball(RenderCommandType type, Material* material, const glm::vec3& pos, RenderFlag flags) {
	assert(type != RenderCommandType::RENDER_COMMAND_TYPE_MODEL);

	if (type == RenderCommandType::RENDER_COMMAND_TYPE_NON_MODEL_METABALL) {
		DXRBase::Metaball ball;
		ball.pos = pos;
		ball.distToCamera = glm::length(ball.pos - camera->getPosition());

		m_metaballs.emplace_back(ball);
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

void DX12RaytracingRenderer::setTeamColors(const std::vector<glm::vec3>& teamColors) {
	Renderer::setTeamColors(teamColors);
}

bool DX12RaytracingRenderer::checkIfOnWater(const glm::vec3& pos) {

	return m_dxr.checkWaterAtWorldPosition(pos);
}

void DX12RaytracingRenderer::updateMetaballAABB() {
	
}

void DX12RaytracingRenderer::setGBufferInputs(DX12RenderableTexture** inputs) {
	m_dxr.setGBufferInputs(inputs);
}

bool DX12RaytracingRenderer::onResize(const WindowResizeEvent& event) {
	m_outputTexture->resize(event.width, event.height);
	m_outputBloomTexture->resize(event.width, event.height);
	return true;
}