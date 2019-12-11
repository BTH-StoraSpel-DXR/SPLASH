#include "pch.h"
#include "DX12RaytracingRenderer.h"
#include "Sail/Application.h"
#include "../DX12Utils.h"
#include "Sail/graphics/postprocessing/PostProcessPipeline.h"
#include "API/DX12/DX12VertexBuffer.h"
#include "Sail/KeyBinds.h"
#include "Sail/entities/components/MapComponent.h"
#include "Sail/graphics/geometry/factory/ScreenQuadModel.h"
#include "Sail/graphics/shader/dxr/ShadePassShader.h"
#include "API/DX12/shader/DX12ShaderPipeline.h"
#include "Sail/graphics/light/LightSetup.h"
#include "Sail/entities/systems/Gameplay/LevelSystem/LevelSystem.h"
#include "Sail/events/EventDispatcher.h"

// Include defines shared with dxr shaders
#include "Sail/../../SPLASH/res/shaders/dxr/Common_hlsl_cpp.hlsl"

// Current goal is to make this render a fully raytraced image of all geometry (without materials) within a scene

DX12RaytracingRenderer::DX12RaytracingRenderer(DX12RenderableTexture** inputs)
	: m_dxr("Basic", inputs)
	, m_gbufferTextures(inputs)
{
	EventDispatcher::Instance().subscribe(Event::Type::WINDOW_RESIZE, this);
	lightSetup = nullptr;
	Application* app = Application::getInstance();
	m_context = app->getAPI<DX12API>();
	m_context->initCommand(m_commandDirectCopy, D3D12_COMMAND_LIST_TYPE_DIRECT, L"Raytracing Renderer copy DIRECT command list or allocator");
	m_context->initCommand(m_commandDirectShading, D3D12_COMMAND_LIST_TYPE_DIRECT, L"Raytracing Renderer shading DIRECT command list or allocator");
	m_context->initCommand(m_commandCompute, D3D12_COMMAND_LIST_TYPE_COMPUTE, L"Raytracing Renderer main COMPUTE command list or allocator");
	m_context->initCommand(m_commandComputePostProcess, D3D12_COMMAND_LIST_TYPE_COMPUTE, L"Raytracing Renderer post process COMPUTE command list or allocator");

	auto windowWidth = app->getWindow()->getWindowWidth();
	auto windowHeight = app->getWindow()->getWindowHeight();

	// Initialize textures used for both hard and soft shadows
	m_outputTextures.positionsOne = std::unique_ptr<DX12RenderableTexture>(static_cast<DX12RenderableTexture*>(RenderableTexture::Create(windowWidth, windowHeight, "PositionOneTexture", Texture::Texture::R32G32B32A32_FLOAT)));
	m_outputBloomTexture = std::unique_ptr<DX12RenderableTexture>(static_cast<DX12RenderableTexture*>(RenderableTexture::Create(windowWidth, windowHeight, "Raytracing renderer bloom output texture", Texture::R16G16B16A16_FLOAT)));
	// Initialize textures used for soft shadows if setting is enabled
	m_hardShadowsLastFrame = Application::getInstance()->getSettings().applicationSettingsStatic["graphics"]["shadows"].getSelected().value == 0.f;
	if (!m_hardShadowsLastFrame) {
		createSoftShadowsTextures(0);
	}

	m_brdfTexture = static_cast<DX12Texture*>(&app->getResourceManager().getTexture("pbr/brdfLUT.tga"));

	m_shadeShader = &app->getResourceManager().getShaderSet<ShadePassShader>();
	m_fullscreenModel = ModelFactory::ScreenQuadModel::Create(m_shadeShader);
}

DX12RaytracingRenderer::~DX12RaytracingRenderer() {
	EventDispatcher::Instance().unsubscribe(Event::Type::WINDOW_RESIZE, this);
}

void DX12RaytracingRenderer::present(PostProcessPipeline* postProcessPipeline, RenderableTexture* output) {
	auto frameIndex = m_context->getSwapIndex();

	if (postProcessPipeline) {
		// Make sure output textures are in a higher precision format to accomodate values > 1
		if (m_shadedOutput) {
			m_shadedOutput->changeFormat(Texture::R16G16B16A16_FLOAT);
		}
		m_outputBloomTexture->changeFormat(Texture::R16G16B16A16_FLOAT);
	} else {
		// Make sure output textures are in a format that can be directly copied to the back buffer
		// Please note that no tone mapping is applied when post process is turned off.
		if (m_shadedOutput) {
			m_shadedOutput->changeFormat(Texture::R8G8B8A8);
		}
		m_outputBloomTexture->changeFormat(Texture::R8G8B8A8);
	}

	// There is one allocator per swap buffer
	auto& allocatorCompute = m_commandCompute.allocators[m_context->getFrameIndex()];
	auto& allocatorComputePostProess = m_commandComputePostProcess.allocators[m_context->getFrameIndex()];
	auto& allocatorDirectCopy = m_commandDirectCopy.allocators[m_context->getFrameIndex()];
	auto& allocatorDirectShading = m_commandDirectShading.allocators[m_context->getFrameIndex()];
	auto& cmdListCompute = m_commandCompute.list;
	auto& cmdListComputePostProcess = m_commandComputePostProcess.list;
	auto& cmdListDirectCopy = m_commandDirectCopy.list;
	auto& cmdListDirectShading = m_commandDirectShading.list;

	// Reset allocators and lists for this frame
	allocatorCompute->Reset();
	allocatorComputePostProess->Reset();
	allocatorDirectCopy->Reset();
	allocatorDirectShading->Reset();
	cmdListCompute->Reset(allocatorCompute.Get(), nullptr);
	cmdListComputePostProcess->Reset(allocatorComputePostProess.Get(), nullptr);
	cmdListDirectCopy->Reset(allocatorDirectCopy.Get(), nullptr);
	cmdListDirectShading->Reset(allocatorDirectShading.Get(), nullptr);

	// Wait for the G-Buffer pass to finish execution on the direct queue
	auto fenceVal = m_context->getDirectQueue()->signal();
	m_context->getComputeQueue()->wait(fenceVal);


	int gpuGroupIndex = 0;
	int gpuGroupStartOffset = 0;
	int nMetaballs = 0;
	bool removeGroups = false;
	std::vector<DXRBase::MetaballGroup*> metaballGroups_vec;

	for (auto& group : m_metaballGroups_map) {
		group.second.averageDistToCamera /= group.second.balls.size();
		metaballGroups_vec.push_back(&group.second);
	}

	//Sort groups by dist to camera
	std::sort(metaballGroups_vec.begin(), metaballGroups_vec.end(),
		[](const DXRBase::MetaballGroup* a, const DXRBase::MetaballGroup* b) -> const bool
		{
			return a->averageDistToCamera < b->averageDistToCamera;
		});

	for (auto& group : metaballGroups_vec) {
		if (gpuGroupIndex >= MAX_NUM_METABALL_GROUPS || nMetaballs >= MAX_NUM_METABALLS) {
			removeGroups = true;
			break;
		}

		//Sort Metaballs by dist to camera
		std::sort(group->balls.begin(), group->balls.end(),
			[](const DXRBase::Metaball& a, const DXRBase::Metaball& b) -> const bool
			{
				return a.distToCamera < b.distToCamera;
			});

		//Calculate the needed size of m_next_metaball_aabb.
		glm::vec3& pos = group->balls.front().pos;

		D3D12_RAYTRACING_AABB nextMetaballAabb;
		nextMetaballAabb.MaxX = pos.x + METABALL_RADIUS;
		nextMetaballAabb.MaxY = pos.y + METABALL_RADIUS;
		nextMetaballAabb.MaxZ = pos.z + METABALL_RADIUS;
		nextMetaballAabb.MinX = pos.x - METABALL_RADIUS;
		nextMetaballAabb.MinY = pos.y - METABALL_RADIUS;
		nextMetaballAabb.MinZ = pos.z - METABALL_RADIUS;
		nMetaballs++;
		size_t size = group->balls.size();
		for (size_t i = 1; i < size; i++) {
			if (nMetaballs >= MAX_NUM_METABALLS) {
				group->balls.erase(group->balls.begin() + i, group->balls.end());
				break;
			}
			glm::vec3& pos = group->balls[i].pos;

			if (nextMetaballAabb.MaxX < pos.x + METABALL_RADIUS) {
				nextMetaballAabb.MaxX = pos.x + METABALL_RADIUS;
			}
			if (nextMetaballAabb.MaxY < pos.y + METABALL_RADIUS) {
				nextMetaballAabb.MaxY = pos.y + METABALL_RADIUS;
			}
			if (nextMetaballAabb.MaxZ < pos.z + METABALL_RADIUS) {
				nextMetaballAabb.MaxZ = pos.z + METABALL_RADIUS;
			}

			if (nextMetaballAabb.MinX > pos.x - METABALL_RADIUS) {
				nextMetaballAabb.MinX = pos.x - METABALL_RADIUS;
			}
			if (nextMetaballAabb.MinY > pos.y - METABALL_RADIUS) {
				nextMetaballAabb.MinY = pos.y - METABALL_RADIUS;
			}
			if (nextMetaballAabb.MinZ > pos.z - METABALL_RADIUS) {
				nextMetaballAabb.MinZ = pos.z - METABALL_RADIUS;
			}

			nMetaballs++;
		}

		group->aabb = nextMetaballAabb;
		group->index = gpuGroupIndex++;
		group->gpuGroupStartOffset = gpuGroupStartOffset;
		gpuGroupStartOffset += group->balls.size();
	}
	//Remove groups that do not fit in memory
	if (removeGroups) {
		metaballGroups_vec.erase(metaballGroups_vec.begin() + gpuGroupIndex, metaballGroups_vec.end());
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

	bool hardShadows = Application::getInstance()->getSettings().applicationSettingsStatic["graphics"]["shadows"].getSelected().value == 0.f;
	if (hardShadows != m_hardShadowsLastFrame) {
		m_context->waitForGPU();
		// Shadow setting changed this frame - handle it
		if (hardShadows) {
			// Destroy all textures but shading and bloom outputs
			m_outputTextures.albedo.reset();
			m_outputTextures.metalnessRoughnessAO.reset();
			m_outputTextures.normal.reset();
			m_outputTextures.shadows.reset();
			m_outputTextures.positionsTwo.reset();
			m_shadedOutput.reset();
			m_shadowsLastFrame.reset();
			
		} else {
			// Create all textures§§
			createSoftShadowsTextures(m_numShadowTextures);
		}
	}
	m_hardShadowsLastFrame = hardShadows;
	
	m_dxr.updateSceneData(camera, lightSetup, metaballGroups_vec, teamColors, m_numShadowTextures);
	m_dxr.updateWaterData();
	m_dxr.updateAccelerationStructures(commandQueue, metaballGroups_vec, cmdListCompute.Get());
	m_dxr.dispatch(m_outputTextures, m_outputBloomTexture.get(), m_shadowsLastFrame.get(), cmdListCompute.Get());
	// AS has now been updated this frame, reset flag
	for (auto& renderCommand : commandQueue) {
		renderCommand.hasUpdatedSinceLastRender[frameIndex] = false;
	}

	DX12RenderableTexture* shadedOutput = m_outputTextures.positionsOne.get(); // Positions is used as the shaded output when running hard shadows
	if (!hardShadows) {
		// Copy output shadow texture to last shadow texture
		m_outputTextures.shadows->transitionStateTo(cmdListCompute.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE);
		m_shadowsLastFrame->transitionStateTo(cmdListCompute.Get(), D3D12_RESOURCE_STATE_COPY_DEST);
		cmdListCompute->CopyResource(m_shadowsLastFrame->getResource(), m_outputTextures.shadows->getResource());
		m_outputTextures.shadows->transitionStateTo(cmdListCompute.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		m_shadowsLastFrame->transitionStateTo(cmdListCompute.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		auto filteredShadows = runDenoising(cmdListCompute.Get());
		shadedOutput = runShading(cmdListDirectShading.Get(), filteredShadows);
	}

	RenderableTexture* renderOutput = shadedOutput;
	if (postProcessPipeline) {
		// Run post processing
		postProcessPipeline->setBloomInput(m_outputBloomTexture.get());
		// Transition shaded output to a state readable by the compute pipeline
		shadedOutput->transitionStateTo(cmdListDirectShading.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		RenderableTexture* ppOutput = postProcessPipeline->run(shadedOutput, cmdListComputePostProcess.Get());
		if (ppOutput) {
			renderOutput = ppOutput;
		}
	}
	DX12RenderableTexture* dxRenderOutput = static_cast<DX12RenderableTexture*>(renderOutput);

	// Execute compute command list to do AS updates and ray dispatching
	cmdListCompute->Close();
	m_context->getComputeQueue()->executeCommandLists({ cmdListCompute.Get() });
	// Place a signal to syncronize copying the raytracing output to the backbuffer when it is available
	fenceVal = m_context->getComputeQueue()->signal();

	// Wait for compute to finish before executing the direct queue to do shading
	m_context->getDirectQueue()->wait(fenceVal);
	// Execute direct command list
	cmdListDirectShading->Close();
	m_context->getDirectQueue()->executeCommandLists({ cmdListDirectShading.Get() });
	fenceVal = m_context->getDirectQueue()->signal();

	// Wait for the direct queue to finish (doing shading) before executing the post processing
	cmdListComputePostProcess->Close();
	m_context->getComputeQueue()->wait(fenceVal);
	m_context->getComputeQueue()->executeCommandLists({ cmdListComputePostProcess.Get() });
	fenceVal = m_context->getComputeQueue()->signal();

	// Copy post processing output to back buffer
	dxRenderOutput->transitionStateTo(cmdListDirectCopy.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE);
	auto* renderTarget = m_context->getCurrentRenderTargetResource();
	DX12Utils::SetResourceTransitionBarrier(cmdListDirectCopy.Get(), renderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
	cmdListDirectCopy->CopyResource(renderTarget, dxRenderOutput->getResource());
	// Lastly - transition back buffer to present
	DX12Utils::SetResourceTransitionBarrier(cmdListDirectCopy.Get(), renderTarget, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);

	cmdListDirectCopy->Close();
	// Wait for the compute queue to finish (doing post processing) befor executing resource copying
	m_context->getDirectQueue()->wait(fenceVal);
	m_context->getDirectQueue()->executeCommandLists({ cmdListDirectCopy.Get() });
}

DX12RenderableTexture* DX12RaytracingRenderer::runDenoising(ID3D12GraphicsCommandList4* cmdList) {
	Application* app = Application::getInstance();
	const auto windowWidth = app->getWindow()->getWindowWidth();
	const auto windowHeight = app->getWindow()->getWindowHeight();

	cmdList->SetComputeRootSignature(m_context->getGlobalRootSignature());
	m_context->getComputeGPUDescriptorHeap()->bind(cmdList);
	
	m_outputTextures.albedo->transitionStateTo(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	auto pass = [&](Shader& shader) {
		PostProcessPipeline::PostProcessInput input;
		auto* settings = shader.getComputeSettings();
		input.outputWidth = static_cast<unsigned int>(windowWidth);
		input.outputHeight = static_cast<unsigned int>(windowHeight);
		input.threadGroupCountX = static_cast<unsigned int>(glm::ceil(settings->threadGroupXScale * input.outputWidth));
		input.threadGroupCountY = static_cast<unsigned int>(glm::ceil(settings->threadGroupYScale * input.outputHeight));
		// Bind the heap
		cmdList->SetComputeRootDescriptorTable(m_context->getRootIndexFromRegister("t0"), m_context->getComputeGPUDescriptorHeap()->getCurentGPUDescriptorHandle());
		// Manually step descriptor heap to where UAVs start
		m_context->getComputeGPUDescriptorHeap()->getAndStepIndex(10);
		// Copy UAV descriptor to the heap
		m_context->getDevice()->CopyDescriptorsSimple(1, m_context->getComputeGPUDescriptorHeap()->getNextCPUDescriptorHandle(), m_outputTextures.shadows->getUavCDH(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		// Do de dispatcheru
		m_computeShaderDispatcher.dispatch(shader, input, cmdList);
	};

	auto& blurShaderHorizontal = app->getResourceManager().getShaderSet<BilateralBlurHorizontal>();
	auto& blurShaderVertical = app->getResourceManager().getShaderSet<BilateralBlurVertical>();

	// Horizontal pass
	pass(blurShaderHorizontal);
	// Vertical pass
	pass(blurShaderVertical);

	return m_outputTextures.shadows.get();
}

DX12RenderableTexture* DX12RaytracingRenderer::runShading(ID3D12GraphicsCommandList4* cmdList, DX12RenderableTexture* shadows) {
	auto shaderPipeline = static_cast<DX12ShaderPipeline*>(m_shadeShader->getPipeline());

	// Make sure model has been initialized
	static_cast<DX12VertexBuffer&>(m_fullscreenModel->getMesh(0)->getVertexBuffer()).init(cmdList);

	m_shadedOutput->transitionStateTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_outputBloomTexture->transitionStateTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Bind shaded and bloom outputs
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2] {
		m_shadedOutput->getRtvCDH(),
		m_outputBloomTexture->getRtvCDH()
	};
	cmdList->OMSetRenderTargets(2, rtvHandles, false, nullptr);
	cmdList->RSSetViewports(1, m_context->getViewport());
	cmdList->RSSetScissorRects(1, m_context->getScissorRect());

	cmdList->SetGraphicsRootSignature(m_context->getGlobalRootSignature());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// Bind the descriptor heap that will contain all SRVs, DSVs and RTVs for this frame
	m_context->getMainGPUDescriptorHeap()->bind(cmdList);

	shaderPipeline->bind(cmdList);

	// The following order must match the t-register order in shader
	int numCustomSRVs = 0;
	shaderPipeline->setTexture2D("albedoBounceOne", m_gbufferTextures[1], cmdList); numCustomSRVs++;
	shaderPipeline->setTexture2D("albedoBounceTwo", m_outputTextures.albedo.get(), cmdList); numCustomSRVs++;

	shaderPipeline->setTexture2D("normalsBounceOne", m_gbufferTextures[0], cmdList); numCustomSRVs++;
	shaderPipeline->setTexture2D("normalsBounceTwo", m_outputTextures.normal.get(), cmdList); numCustomSRVs++;

	shaderPipeline->setTexture2D("metalnessRoughnessAoBounceOne", m_gbufferTextures[2], cmdList); numCustomSRVs++;
	shaderPipeline->setTexture2D("metalnessRoughnessAoBounceTwo", m_outputTextures.metalnessRoughnessAO.get(), cmdList); numCustomSRVs++;

	shaderPipeline->setTexture2D("shadows", shadows, cmdList); numCustomSRVs++;
	
	shaderPipeline->setTexture2D("positionsOne", m_outputTextures.positionsOne.get(), cmdList); numCustomSRVs++;
	shaderPipeline->setTexture2D("positionsTwo", m_outputTextures.positionsTwo.get(), cmdList); numCustomSRVs++;

	shaderPipeline->setTexture2D("brdfLUT", m_brdfTexture, cmdList); numCustomSRVs++;

	if (camera) {
		shaderPipeline->setCBufferVar("cameraPosition", &camera->getPosition(), sizeof(glm::vec3));
	}
	if (lightSetup) {
		auto& plData = lightSetup->getPointLightsData();
		auto& slData = lightSetup->getSpotLightsData();
		shaderPipeline->setCBufferVar("pointLights", &plData, sizeof(plData));
		shaderPipeline->setCBufferVar("spotLights", &slData, sizeof(slData));

		const unsigned int maxLights = NUM_TOTAL_LIGHTS; // Point light and spot lights
		IndexMap indexMapData[maxLights];
		// Map shadow texture indices to light indices
		uint lightIndex = 0;
		uint shadowTextureIndex = 0;
		for (auto& pl : plData.pLights) {
			if (pl.color == glm::vec3(0.f)) {
				// No shadow texture when light color is black/disabled
				indexMapData[lightIndex].index = -1;
			} else {
				indexMapData[lightIndex].index = shadowTextureIndex++;
			}
			lightIndex++;
		}
		for (auto& sl : slData.sLights) {
			if (sl.color == glm::vec3(0.f)) {
				// No shadow texture when light color is black/disabled
				indexMapData[lightIndex].index = -1;
			} else {
				indexMapData[lightIndex].index = shadowTextureIndex++;
			}
			lightIndex++;
		}
		shaderPipeline->setCBufferVar("shadowTextureIndexMap", &indexMapData, sizeof(indexMapData));
		shaderPipeline->setCBufferVar("numShadowTextures", &m_numShadowTextures, sizeof(unsigned int));
	}

	static_cast<DX12Mesh*>(m_fullscreenModel->getMesh(0))->draw_new(*this, cmdList, -numCustomSRVs);

	// setTexture2D transitions texture to pixel shader resource
	// We need to transition them back to a state that is usable on a compute queue before the next frame
	// TODO: batch transitions
	m_outputTextures.albedo->transitionStateTo(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_outputTextures.normal->transitionStateTo(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_outputTextures.metalnessRoughnessAO->transitionStateTo(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_outputTextures.positionsOne->transitionStateTo(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_outputTextures.positionsTwo->transitionStateTo(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_outputBloomTexture->transitionStateTo(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
	shadows->transitionStateTo(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_brdfTexture->transitionStateTo(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);

	return m_shadedOutput.get();
}

void DX12RaytracingRenderer::begin(Camera* camera) {
	Renderer::begin(camera);
	m_metaballGroups_map.clear();
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

void DX12RaytracingRenderer::submit(Mesh* mesh, const glm::mat4& modelMatrix, RenderFlag flags, int teamColorID, bool castShadows) {
	RenderCommand cmd;
	cmd.type = RENDER_COMMAND_TYPE_MODEL;
	cmd.model.mesh = mesh;
	cmd.transform = glm::transpose(modelMatrix);
	cmd.flags = flags;
	cmd.teamColorID = teamColorID;
	cmd.castShadows = castShadows;
	// Resize to match numSwapBuffers (specific to dx12)
	cmd.hasUpdatedSinceLastRender.resize(m_context->getNumGPUBuffers(), false);
	commandQueue.push_back(cmd);
}

void DX12RaytracingRenderer::submitMetaball(RenderCommandType type, Material* material, const glm::vec3& pos, RenderFlag flags, int group) {
	assert(type != RenderCommandType::RENDER_COMMAND_TYPE_MODEL);

	if (type == RenderCommandType::RENDER_COMMAND_TYPE_NON_MODEL_METABALL) {
		DXRBase::Metaball ball;
		ball.pos = pos;
		ball.distToCamera = glm::length(ball.pos - camera->getPosition());

		m_metaballGroups_map[group].balls.emplace_back(ball);
		m_metaballGroups_map[group].averageDistToCamera += ball.distToCamera;
	}
}

void DX12RaytracingRenderer::submitWaterPoint(const glm::vec3& pos) {
	m_dxr.addWaterAtWorldPosition(pos);
}

unsigned int DX12RaytracingRenderer::removeWaterPoint(const glm::vec3& pos, const glm::ivec3& posOffset, const glm::ivec3& negOffset) {
	return m_dxr.removeWaterAtWorldPosition(pos, posOffset, negOffset);
}

void DX12RaytracingRenderer::setTeamColors(const std::vector<glm::vec3>& teamColors) {
	Renderer::setTeamColors(teamColors);
}

bool DX12RaytracingRenderer::checkIfOnWater(const glm::vec3& pos) {

	return m_dxr.checkWaterAtWorldPosition(pos);
}

std::pair<bool, glm::vec3> DX12RaytracingRenderer::getNearestWaterPosition(const glm::vec3& position, const glm::vec3& maxOffset) {
	return m_dxr.getNearestWaterPosition(position, maxOffset);
}

void DX12RaytracingRenderer::updateMetaballAABB() {
	
}

void DX12RaytracingRenderer::setGBufferInputs(DX12RenderableTexture** inputs) {
	m_dxr.setGBufferInputs(inputs);
}

DXRBase* DX12RaytracingRenderer::getDXRBase() {
	return &m_dxr;
}

void DX12RaytracingRenderer::createSoftShadowsTextures(unsigned int numPlayers) {
	m_numShadowTextures = glm::min<unsigned int>(NUM_TOTAL_LIGHTS - LightSetup::MAX_POINTLIGHTS_RENDERING + numPlayers, NUM_SHADOW_TEXTURES);

	Application* app = Application::getInstance();
	auto windowWidth = app->getWindow()->getWindowWidth();
	auto windowHeight = app->getWindow()->getWindowHeight();
	// Initialize raytracing output textures
	// m_outputTextures contains all reflection bounce information
	// gbuffer textures contains first "bounce" information
	m_outputTextures.albedo = std::unique_ptr<DX12RenderableTexture>(static_cast<DX12RenderableTexture*>(RenderableTexture::Create(windowWidth, windowHeight, "RT albedo secound boucne output")));
	m_outputTextures.metalnessRoughnessAO = std::unique_ptr<DX12RenderableTexture>(static_cast<DX12RenderableTexture*>(RenderableTexture::Create(windowWidth, windowHeight, "RT mrao secound bounce output")));
	m_outputTextures.normal = std::unique_ptr<DX12RenderableTexture>(static_cast<DX12RenderableTexture*>(RenderableTexture::Create(windowWidth, windowHeight, "RT normal secound boucne output")));
	m_outputTextures.shadows = std::unique_ptr<DX12RenderableTexture>(static_cast<DX12RenderableTexture*>(RenderableTexture::Create(windowWidth, windowHeight, "CurrentFrameShadowTexture", Texture::R8G8, false, false, m_numShadowTextures)));
	m_outputTextures.positionsTwo = std::unique_ptr<DX12RenderableTexture>(static_cast<DX12RenderableTexture*>(RenderableTexture::Create(windowWidth, windowHeight, "PositionTwoTexture", Texture::R32G32B32A32_FLOAT)));
	// init shaded output texture - this is written to in a rasterisation pass
	m_shadedOutput = std::unique_ptr<DX12RenderableTexture>(static_cast<DX12RenderableTexture*>(RenderableTexture::Create(windowWidth, windowHeight, "RT shaded output")));
	// Init raytracing input texture
	m_shadowsLastFrame = std::unique_ptr<DX12RenderableTexture>(static_cast<DX12RenderableTexture*>(RenderableTexture::Create(windowWidth, windowHeight, "LastFrameShadowTexture", Texture::R8G8, false, false, m_numShadowTextures)));
}

bool DX12RaytracingRenderer::onResize(const WindowResizeEvent& event) {
	if (m_outputTextures.albedo)				{ m_outputTextures.albedo->resize(event.width, event.height); }
	if (m_outputTextures.normal)				{ m_outputTextures.normal->resize(event.width, event.height); }
	if (m_outputTextures.metalnessRoughnessAO)	{ m_outputTextures.metalnessRoughnessAO->resize(event.width, event.height); }
	if (m_outputTextures.shadows)				{ m_outputTextures.shadows->resize(event.width, event.height); }
	if (m_outputTextures.positionsOne)			{ m_outputTextures.positionsOne->resize(event.width, event.height); }
	if (m_outputTextures.positionsTwo)			{ m_outputTextures.positionsTwo->resize(event.width, event.height); }
	if (m_shadowsLastFrame)						{ m_shadowsLastFrame->resize(event.width, event.height); }
	if (m_shadedOutput)							{ m_shadedOutput->resize(event.width, event.height); }
	if (m_outputBloomTexture)					{ m_outputBloomTexture->resize(event.width, event.height); }
	return true;
}
