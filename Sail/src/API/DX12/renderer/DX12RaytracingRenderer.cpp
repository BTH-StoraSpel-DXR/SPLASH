#include "pch.h"
#include "DX12RaytracingRenderer.h"
#include "Sail/Application.h"
#include "../DX12Utils.h"
#include "Sail/graphics/postprocessing/PostProcessPipeline.h"

// Current goal is to make this render a fully raytraced image of all geometry (without materials) within a scene

DX12RaytracingRenderer::DX12RaytracingRenderer() 
	: m_dxr("Basic")
{
	Application* app = Application::getInstance();
	m_context = app->getAPI<DX12API>();
	m_context->initCommand(m_command);
	m_command.list->SetName(L"Raytracing Renderer main command list");

	auto windowWidth = app->getWindow()->getWindowWidth();
	auto windowHeight = app->getWindow()->getWindowHeight();
	m_outputTexture = std::unique_ptr<DX12RenderableTexture>(static_cast<DX12RenderableTexture*>(RenderableTexture::Create(windowWidth, windowHeight)));
}

DX12RaytracingRenderer::~DX12RaytracingRenderer() {

}

void DX12RaytracingRenderer::present(PostProcessPipeline* postProcessPipeline, RenderableTexture* output) {
	auto frameIndex = m_context->getFrameIndex();

	auto& allocator = m_command.allocators[frameIndex];
	auto& cmdList = m_command.list;

	// Reset allocators and lists for this frame
	allocator->Reset();
	cmdList->Reset(allocator.Get(), nullptr);

	m_dxr.updateAccelerationStructures(commandQueue, cmdList.Get());
	m_dxr.updateCamera(*camera);
	m_dxr.dispatch(m_outputTexture.get(), cmdList.Get());

	// TODO: move this to a graphics queue when current cmdList is executed on the compute queue

	RenderableTexture* renderOutput = m_outputTexture.get();
	if (postProcessPipeline) {
		// Run post processing
		RenderableTexture* ppOutput = postProcessPipeline->run(m_outputTexture.get(), cmdList.Get());
		if (ppOutput) {
			renderOutput = ppOutput;
		}
	}
	DX12RenderableTexture* dxRenderOutput = static_cast<DX12RenderableTexture*>(renderOutput);
	// Copy post processing output to back buffer
	dxRenderOutput->transitionStateTo(cmdList.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE);
	auto* renderTarget = m_context->getCurrentRenderTargetResource();
	DX12Utils::SetResourceTransitionBarrier(cmdList.Get(), renderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
	cmdList->CopyResource(renderTarget, dxRenderOutput->getResource());
	// Lastly - transition back buffer to present
	DX12Utils::SetResourceTransitionBarrier(cmdList.Get(), renderTarget, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);


	// Execute command list
	cmdList->Close();
	m_context->executeCommandLists({ cmdList.Get() });
}

bool DX12RaytracingRenderer::onEvent(Event& event) {
	EventHandler::dispatch<WindowResizeEvent>(event, SAIL_BIND_EVENT(&DX12RaytracingRenderer::onResize));

	// Pass along events
	m_dxr.onEvent(event);
	return true;
}

bool DX12RaytracingRenderer::onResize(WindowResizeEvent& event) {
	m_outputTexture->resize(event.getWidth(), event.getHeight());
	return true;
}