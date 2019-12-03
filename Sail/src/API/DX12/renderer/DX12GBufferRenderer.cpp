#include "pch.h"
#include "DX12GBufferRenderer.h"
#include "Sail/Application.h"
#include "Sail/api/shader/ShaderPipeline.h"
#include "Sail/api/ComputeShaderDispatcher.h"
#include "Sail/graphics/light/LightSetup.h"
#include "Sail/graphics/shader/Shader.h"
#include "Sail/graphics/postprocessing/PostProcessPipeline.h"
#include "../resources/DescriptorHeap.h"
#include "../resources/DX12Texture.h"
#include "../resources/DX12RenderableTexture.h"
#include "../shader/DX12ShaderPipeline.h"
#include "../DX12Mesh.h"
#include "../DX12Utils.h"
#include "Sail/entities/systems/Graphics/AnimationSystem.h"
#include "Sail/entities/systems/Graphics/ParticleSystem.h"
#include "Sail/entities/components/RenderInActiveGameComponent.h"
#include "Sail/entities/components/RenderInReplayComponent.h"
#include "Sail/entities/ECS.h"
#include "../DX12VertexBuffer.h"
#include "Sail/entities/systems/physics/OctreeAddRemoverSystem.h"
#include "Sail/events/EventDispatcher.h"
#include "API/DX12/resources/DX12DDSTexture.h"

DX12GBufferRenderer::DX12GBufferRenderer() {
	EventDispatcher::Instance().subscribe(Event::Type::WINDOW_RESIZE, this);

	Application* app = Application::getInstance();
	m_context = app->getAPI<DX12API>();

	for (size_t i = 0; i < MAX_RECORD_THREADS; i++) {
		std::wstring name = L"GBuffer renderer DIRECT command list or allocator for render thread: " + std::to_wstring(i);
		m_context->initCommand(m_command[i], D3D12_COMMAND_LIST_TYPE_DIRECT, name.c_str());
	}
	m_context->initCommand(m_computeCommand, D3D12_COMMAND_LIST_TYPE_COMPUTE, L"GBuffer renderer COMPUTE command list or allocator");
	m_computeCommand.list->SetName(L"Animation compute command list");

	auto windowWidth = app->getWindow()->getWindowWidth();
	auto windowHeight = app->getWindow()->getWindowHeight();

	for (int i = 0; i < NUM_GBUFFERS; i++) {
		m_gbufferTextures[i] = static_cast<DX12RenderableTexture*>(RenderableTexture::Create(windowWidth, windowHeight, "GBuffer renderer output " + std::to_string(i), Texture::R8G8B8A8, (i == 0)));
	}
}

DX12GBufferRenderer::~DX12GBufferRenderer() {
	for (int i = 0; i < NUM_GBUFFERS; i++) {
		delete m_gbufferTextures[i];
	}
	EventDispatcher::Instance().unsubscribe(Event::Type::WINDOW_RESIZE, this);
}

void DX12GBufferRenderer::present(PostProcessPipeline* postProcessPipeline, RenderableTexture* output) {
	assert(!output); // Render to texture is currently not implemented for DX12!

	auto frameIndex = m_context->getFrameIndex();
	int count = static_cast<int>(commandQueue.size());

	auto* animationSystem = ECS::Instance()->getSystem<AnimationSystem<RenderInActiveGameComponent>>();
	auto* killCamAnimationSystem = ECS::Instance()->getSystem<AnimationSystem<RenderInReplayComponent>>();
	auto* particleSystem = ECS::Instance()->getSystem<ParticleSystem>();
	if (animationSystem || particleSystem) {
		m_computeCommand.allocators[frameIndex]->Reset();
		m_computeCommand.list->Reset(m_computeCommand.allocators[frameIndex].Get(), nullptr);

		if (animationSystem) {
			// Run animation updates on the gpu first
			animationSystem->updateMeshGPU(m_computeCommand.list.Get());
		}
		if (killCamAnimationSystem) {
			// Run animation updates on the gpu first
			killCamAnimationSystem->updateMeshGPU(m_computeCommand.list.Get());
		}
		if (particleSystem) {
			// Update particles on compute shader
			particleSystem->updateOnGPU(m_computeCommand.list.Get(), camera->getPosition());
		}

		m_computeCommand.list->Close();
		m_context->getComputeQueue()->executeCommandLists({ m_computeCommand.list.Get() });
		// Force direct queue to wait until the compute queue has finished animations
		m_context->getDirectQueue()->wait(m_context->getComputeQueue()->signal());
	}

#ifdef MULTI_THREADED_COMMAND_RECORDING

	int nThreadsToUse = (count / MIN_COMMANDS_PER_THREAD) + (count % MIN_COMMANDS_PER_THREAD > 0);
	if (nThreadsToUse < 0) {
		nThreadsToUse = 1;
	} else if (nThreadsToUse > MAX_RECORD_THREADS) {
		nThreadsToUse = MAX_RECORD_THREADS;
	}
	nThreadsToUse = glm::max(1, nThreadsToUse);

	std::future<void> fut[MAX_RECORD_THREADS];

	int mainThreadIndex = nThreadsToUse - 1;
	int commandsPerThread = (int)round(count / (float)nThreadsToUse);
	int start = 0;

	for (int i = 0; i < mainThreadIndex; i++) {
		fut[i] = Application::getInstance()->pushJobToThreadPool(
			[this, postProcessPipeline, i, frameIndex, start, commandsPerThread, count, nThreadsToUse](int id) {
			return this->recordCommands(postProcessPipeline, i, frameIndex, start, commandsPerThread, count, nThreadsToUse);
		});
		start += commandsPerThread;
#ifdef DEBUG_MULTI_THREADED_COMMAND_RECORDING
		fut[i].get(); //Force recording threads to be recorded in order
#endif // DEBUG_MULTI_THREADED_COMMAND_RECORDING
	}

	recordCommands(postProcessPipeline, mainThreadIndex, frameIndex, start, commandsPerThread + 1 /* +1 to account for rounding */, count, nThreadsToUse);
	ID3D12CommandList* commandlists[MAX_RECORD_THREADS];
	commandlists[mainThreadIndex] = m_command[mainThreadIndex].list.Get();

	for (size_t i = 0; i < mainThreadIndex; i++) {
#ifndef DEBUG_MULTI_THREADED_COMMAND_RECORDING
		fut[i].get(); //Wait for all recording threads to finish
#endif // DEBUG_MULTI_THREADED_COMMAND_RECORDING
		commandlists[i] = m_command[i].list.Get();
	}
	m_context->getDirectQueue()->executeCommandLists(commandlists, nThreadsToUse);
#else
	recordCommands(0, frameIndex, 0, count, count, 1);
	m_context->executeCommandLists({ m_command[0].list.Get() });
#endif // MULTI_THREADED_COMMAND_RECORDING
}

void DX12GBufferRenderer::recordCommands(PostProcessPipeline* postProcessPipeline, const int threadID, const int frameIndex, const int start, const int nCommands, unsigned int oobMax, int nThreads) {
	assert(!postProcessPipeline); // Post process not allowed on the GBuffer pass!

	auto& allocator = m_command[threadID].allocators[frameIndex];
	auto& cmdList = m_command[threadID].list;

	// Reset allocators and lists for this frame
	allocator->Reset();
	cmdList->Reset(allocator.Get(), nullptr);

	// Bind gbuffer RTV and DSV
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[NUM_GBUFFERS];
	for (int i = 0; i < NUM_GBUFFERS; i++) {
		rtvHandles[i] = m_gbufferTextures[i]->getRtvCDH();
	}
	cmdList->OMSetRenderTargets(NUM_GBUFFERS, rtvHandles, false, &m_gbufferTextures[0]->getDsvCDH());

	cmdList->RSSetViewports(1, m_context->getViewport());
	cmdList->RSSetScissorRects(1, m_context->getScissorRect());


#ifdef MULTI_THREADED_COMMAND_RECORDING
#ifdef DEBUG_MULTI_THREADED_COMMAND_RECORDING
	if (threadID == 0) {
		// TODO: fix
		m_context->prepareToRender(cmdList.Get());
		m_context->clear(cmdList.Get());
		SAIL_LOG("ThreadID: " + std::to_string(threadID) + " - Prep to render, and record. " + std::to_string(start) + " to " + std::to_string(start + nCommands));
	} else if (threadID < nThreads - 1) {
		SAIL_LOG("ThreadID: " + std::to_string(threadID) + " - Recording Only. " + std::to_string(start) + " to " + std::to_string(start + nCommands));
	}
#else
	if (threadID == 0) {

		// Init all vbuffers and textures - this needs to be done on ONE thread
		// TODO: optimize!
		for (auto& renderCommand : commandQueue) {
			auto& vbuffer = static_cast<DX12VertexBuffer&>(renderCommand.model.mesh->getVertexBuffer());
			vbuffer.init(cmdList.Get());
			for (int i = 0; i < 3; i++) {
				auto* tex = static_cast<DX12Texture*>(renderCommand.model.mesh->getMaterial()->getTexture(i));
				if (tex) {
					tex->initBuffers(cmdList.Get());
				}
			}
		}

		// Transition output textures to render target
		for (int i = 0; i < NUM_GBUFFERS; i++) {
			// TODO: transition in batch
			m_gbufferTextures[i]->transitionStateTo(cmdList.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
			m_gbufferTextures[i]->clear({ 0.01f, 0.01f, 0.01f, 1.0f }, cmdList.Get());
		}

	}
#endif // DEBUG_MULTI_THREADED_COMMAND_RECORDING
#else
	m_context->prepareToRender(cmdList.Get());
	m_context->clear(cmdList.Get());
#endif

	cmdList->SetGraphicsRootSignature(m_context->getGlobalRootSignature());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Bind the descriptor heap that will contain all SRVs, DSVs and RTVs for this frame
	m_context->getMainGPUDescriptorHeap()->bind(cmdList.Get());

	// Bind mesh-common constant buffers (camera)
	// TODO: bind camera cbuffer here
	//cmdList->SetGraphicsRootConstantBufferView(GlobalRootParam::CBV_CAMERA, asdf);

	// TODO: Sort meshes according to material
	RenderCommand* command;
	unsigned int commandIndex = start;
	for (int i = 0; i < nCommands && commandIndex < oobMax; i++, commandIndex++) {
		command = &commandQueue[commandIndex];
		DX12ShaderPipeline* shaderPipeline = static_cast<DX12ShaderPipeline*>(command->model.mesh->getMaterial()->getShader()->getPipeline());

		shaderPipeline->checkBufferSizes(oobMax); //Temp fix to expand constant buffers if the scene contain to many objects
		shaderPipeline->bind(cmdList.Get());

		// Used in most shaders
		shaderPipeline->trySetCBufferVar("sys_mWorld", &glm::transpose(command->transform), sizeof(glm::mat4));
		shaderPipeline->trySetCBufferVar("sys_mView", &camera->getViewMatrix(), sizeof(glm::mat4));
		shaderPipeline->trySetCBufferVar("sys_mProj", &camera->getProjMatrix(), sizeof(glm::mat4));

		if (teamColors.size() > 0) {
			cmdList->SetGraphicsRoot32BitConstants(GlobalRootParam::CBV_TEAM_COLOR, 3, &teamColors[command->teamColorID], 0);
		}

		command->model.mesh->draw(*this, cmdList.Get());
	}

	// Lastly - transition back buffer to present
#ifdef MULTI_THREADED_COMMAND_RECORDING
	if (threadID == nThreads - 1) {

		// Transition output textures for use in raytracing
		for (int i = 0; i < NUM_GBUFFERS; i++) {
			// TODO: transition in batch
			m_gbufferTextures[i]->transitionStateTo(cmdList.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}

#ifdef DEBUG_MULTI_THREADED_COMMAND_RECORDING
		SAIL_LOG("ThreadID: " + std::to_string(threadID) + " - Record and prep to present. " + std::to_string(start) + " to " + std::to_string(start + nCommands));
#endif // DEBUG_MULTI_THREADED_COMMAND_RECORDING
	}
#else
	// TODO: dont duplicate this from above
	bool usePostProcessOutput = false;
	if (postProcessPipeline) {
		// Run post processing
		RenderableTexture* renderOutput = postProcessPipeline->run(m_outputTexture.get(), cmdList.Get());
		if (renderOutput) {
			usePostProcessOutput = true;

			DX12RenderableTexture* dxRenderOutput = static_cast<DX12RenderableTexture*>(renderOutput);
			// Copy post processing output to back buffer
			dxRenderOutput->transitionStateTo(cmdList.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE);
			auto* renderTarget = m_context->getCurrentRenderTargetResource();
			DX12Utils::SetResourceTransitionBarrier(cmdList.Get(), renderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
			cmdList->CopyResource(renderTarget, dxRenderOutput->getResource());
			// Lastly - transition back buffer to present
			DX12Utils::SetResourceTransitionBarrier(cmdList.Get(), renderTarget, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
		}
	}
	if (!usePostProcessOutput) {
		m_context->prepareToPresent(cmdList.Get());
	}

#endif
	// Close command list
	cmdList->Close();

}

bool DX12GBufferRenderer::onEvent(const Event& event) {
	switch (event.type) {
	case Event::Type::WINDOW_RESIZE: onResize((const WindowResizeEvent&)event); break;
	default: break;
	}
	return true;
}

DX12RenderableTexture** DX12GBufferRenderer::getGBufferOutputs() const {
	return (DX12RenderableTexture**)m_gbufferTextures;
}

bool DX12GBufferRenderer::onResize(const WindowResizeEvent& event) {
	for (int i = 0; i < NUM_GBUFFERS; i++) {
		m_gbufferTextures[i]->resize(event.width, event.height);
	}
	return true;
}