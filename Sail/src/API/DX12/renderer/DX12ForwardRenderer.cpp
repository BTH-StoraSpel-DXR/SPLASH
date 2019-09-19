#include "pch.h"
#include "DX12ForwardRenderer.h"
#include "Sail/api/shader/ShaderPipeline.h"
#include "Sail/graphics/light/LightSetup.h"
#include "Sail/graphics/shader/Shader.h"
#include "Sail/Application.h"
#include "../DX12Utils.h"
#include "../shader/DX12ShaderPipeline.h"
#include "../DX12Mesh.h"
#include "../resources/DescriptorHeap.h"
#include "Sail/graphics/shader/compute/TestComputeShader.h"
#include "Sail/api/ComputeShaderDispatcher.h"
#include "../resources/DX12Texture.h"
#include "Sail/graphics/postprocessing/PostProcessPipeline.h"
#include "API/DX12/resources/DX12RenderableTexture.h"

DX12ForwardRenderer::DX12ForwardRenderer() {
	Application* app = Application::getInstance();
	m_context = app->getAPI<DX12API>();

	for (size_t i = 0; i < MAX_RECORD_THREADS; i++) {
		m_context->initCommand(m_command[i]);
		std::wstring name = L"Forward Renderer main command list for render thread: " + std::to_wstring(i);
		m_command[i].list->SetName(name.c_str());
	}


	auto windowWidth = app->getWindow()->getWindowWidth();
	auto windowHeight = app->getWindow()->getWindowHeight();
	m_outputTexture = std::unique_ptr<DX12RenderableTexture>(static_cast<DX12RenderableTexture*>(RenderableTexture::Create(windowWidth, windowHeight)));
	m_outputTexture->renameBuffer("Forward renderer output renderable texture");
}

DX12ForwardRenderer::~DX12ForwardRenderer() {

}

void DX12ForwardRenderer::present(PostProcessPipeline* postProcessPipeline, RenderableTexture* output) {
	assert(!output); // Render to texture is currently not implemented for DX12!

	auto frameIndex = m_context->getFrameIndex();
	size_t count = commandQueue.size();

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
	int commandsPerThread = round(count / (float)nThreadsToUse);
	int start = 0;

	for (size_t i = 0; i < mainThreadIndex; i++) {
		fut[i] = Application::getInstance()->pushJobToThreadPool(
			[this, postProcessPipeline, i, frameIndex, start, commandsPerThread, count, nThreadsToUse](int id) {
				return this->recordCommands(postProcessPipeline, i, frameIndex, start, (i < nThreadsToUse - 1) ? commandsPerThread : commandsPerThread + 1, count, nThreadsToUse);
			});
		start += commandsPerThread;
#ifdef DEBUG_MULTI_THREADED_COMMAND_RECORDING
		fut[i].get(); //Force recording threads to be recorded in order
#endif // DEBUG_MULTI_THREADED_COMMAND_RECORDING
	}

	recordCommands(postProcessPipeline, mainThreadIndex, frameIndex, start, commandsPerThread, count, nThreadsToUse);
	ID3D12CommandList* commandlists[MAX_RECORD_THREADS];
	commandlists[mainThreadIndex] = m_command[mainThreadIndex].list.Get();

	for (size_t i = 0; i < mainThreadIndex; i++) {
#ifndef DEBUG_MULTI_THREADED_COMMAND_RECORDING
		fut[i].get(); //Wait for all recording threads to finnish
#endif // DEBUG_MULTI_THREADED_COMMAND_RECORDING
		commandlists[i] = m_command[i].list.Get();
	}
	m_context->executeCommandLists(commandlists, nThreadsToUse);
#else
	recordCommands(0, frameIndex, 0, count, count, 1);
	m_context->executeCommandLists({ m_command[0].list.Get() });
#endif // MULTI_THREADED_COMMAND_RECORDING
}

void DX12ForwardRenderer::recordCommands(PostProcessPipeline* postProcessPipeline, const int threadID, const int frameIndex, const int start, const int nCommands, size_t oobMax, int nThreads) {
	//#ifdef MULTI_THREADED_COMMAND_RECORDING
	auto& allocator = m_command[threadID].allocators[frameIndex];
	auto& cmdList = m_command[threadID].list;

	// Reset allocators and lists for this frame
	allocator->Reset();
	cmdList->Reset(allocator.Get(), nullptr);

	// All threads must bind the render target
	if (postProcessPipeline) {
		m_outputTexture->begin(cmdList.Get());
	} else {
		m_context->renderToBackBuffer(cmdList.Get());
	}

#ifdef MULTI_THREADED_COMMAND_RECORDING
#ifdef DEBUG_MULTI_THREADED_COMMAND_RECORDING
	if (threadID == 0) {
		// TODO: fix
		m_context->prepareToRender(cmdList.Get());
		m_context->clear(cmdList.Get());
		Logger::Log("ThreadID: " + std::to_string(threadID) + " - Prep to render, and record. " + std::to_string(start) + " to " + std::to_string(start + nCommands));
	} else if (threadID < nThreads - 1) {
		Logger::Log("ThreadID: " + std::to_string(threadID) + " - Recording Only. " + std::to_string(start) + " to " + std::to_string(start + nCommands));
	}
#else
	if (threadID == 0) {

		// Init all textures - this needs to be done on ONE thread
		// TODO: optimize!
		for (auto& renderCommand : commandQueue) {
			for (int i = 0; i < 3; i++) {
				auto* tex = static_cast<DX12Texture*>(renderCommand.mesh->getMaterial()->getTexture(i));
				if (tex && !tex->hasBeenInitialized()) {
					tex->initBuffers(cmdList.Get());
				}
			}
		}


		// Transition output texture to render target
		if (postProcessPipeline) {
			m_outputTexture->transitionStateTo(cmdList.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
			m_outputTexture->clear({ 0.1f, 0.2f, 0.3f, 1.0f }, cmdList.Get());
		} else {
			m_context->prepareToRender(cmdList.Get());
			m_context->clear(cmdList.Get(), { 0.1f, 0.2f, 0.3f, 1.0f });
		}
		
	}
#endif // DEBUG_MULTI_THREADED_COMMAND_RECORDING
#else
	m_context->prepareToRender(cmdList.Get());
	m_context->clear(cmdList.Get());
#endif

	cmdList->SetGraphicsRootSignature(m_context->getGlobalRootSignature());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Bind the descriptor heap that will contain all SRVs for this frame
	m_context->getMainGPUDescriptorHeap()->bind(cmdList.Get());

	// Bind mesh-common constant buffers (camera)
	// TODO: bind camera cbuffer here
	//cmdList->SetGraphicsRootConstantBufferView(GlobalRootParam::CBV_CAMERA, asdf);

	// TODO: Sort meshes according to material
	unsigned int meshIndex = start;
	RenderCommand* command;
	for (int i = 0; i < nCommands && meshIndex < oobMax; i++, meshIndex++ /*RenderCommand& command : commandQueue*/) {
		command = &commandQueue[meshIndex];
		DX12ShaderPipeline* shaderPipeline = static_cast<DX12ShaderPipeline*>(command->mesh->getMaterial()->getShader()->getPipeline());

		shaderPipeline->checkBufferSizes(oobMax); //Temp fix to expand constant buffers if the scene contain to many objects
		shaderPipeline->bind_new(cmdList.Get(), meshIndex);

		shaderPipeline->setCBufferVar_new("sys_mWorld", &glm::transpose(command->transform), sizeof(glm::mat4), meshIndex);
		shaderPipeline->setCBufferVar_new("sys_mVP", &camera->getViewProjection(), sizeof(glm::mat4), meshIndex);
		shaderPipeline->setCBufferVar_new("sys_cameraPos", &camera->getPosition(), sizeof(glm::vec3), meshIndex);

		if (lightSetup) {
			auto& dlData = lightSetup->getDirLightData();
			auto& plData = lightSetup->getPointLightsData();
			shaderPipeline->setCBufferVar_new("dirLight", &dlData, sizeof(dlData), meshIndex);
			shaderPipeline->setCBufferVar_new("pointLights", &plData, sizeof(plData), meshIndex);
		}

		static_cast<DX12Mesh*>(command->mesh)->draw_new(*this, cmdList.Get(), meshIndex);
	}

	// Lastly - transition back buffer to present
#ifdef MULTI_THREADED_COMMAND_RECORDING
	if (threadID == nThreads - 1) {
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
#ifdef DEBUG_MULTI_THREADED_COMMAND_RECORDING
		Logger::Log("ThreadID: " + std::to_string(threadID) + " - Record and prep to present. " + std::to_string(start) + " to " + std::to_string(start + nCommands));
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

bool DX12ForwardRenderer::onEvent(Event& event) {
	EventHandler::dispatch<WindowResizeEvent>(event, SAIL_BIND_EVENT(&DX12ForwardRenderer::onResize));
	return true;
}

bool DX12ForwardRenderer::onResize(WindowResizeEvent& event) {
	m_outputTexture->resize(event.getWidth(), event.getHeight());
	return true;
}