#include "pch.h"
#include "DX12ParticleRenderer.h"
#include "Sail/Application.h"
#include "API/DX12/resources/DX12Texture.h"
#include "API/DX12/shader/DX12ShaderPipeline.h"
#include "API/DX12/DX12Mesh.h"
#include "Sail/graphics/geometry/PBRMaterial.h"
#include "Sail/graphics/shader/Shader.h"
#include "API/DX12/DX12VertexBuffer.h"
#include "API/DX12/resources/DX12RenderableTexture.h"
#include "DX12GBufferRenderer.h"
#include "DX12HybridRaytracerRenderer.h"

DX12ParticleRenderer::DX12ParticleRenderer() {
	Application* app = Application::getInstance();
	m_context = app->getAPI<DX12API>();

	m_context->initCommand(m_command, D3D12_COMMAND_LIST_TYPE_DIRECT, L"ScreenSpace Renderer main command list");
	
	auto windowWidth = app->getWindow()->getWindowWidth();
	auto windowHeight = app->getWindow()->getWindowHeight();

	// Tell particle renderer to use depth output from gbuffer renderer
	auto* gbuffers = static_cast<DX12HybridRaytracerRenderer*>(app->getRenderWrapper()->getCurrentRenderer())->getGBufferRenderer()->getGBufferOutputs();
	m_depthTexture = gbuffers[0];
}

DX12ParticleRenderer::~DX12ParticleRenderer() {}

void DX12ParticleRenderer::present(PostProcessPipeline* postProcessPipeline, RenderableTexture* output) {
	assert(!output); // Render to texture is currently not implemented for DX12!

	auto frameIndex = m_context->getSwapIndex();

	auto& allocator = m_command.allocators[m_context->getFrameIndex()];
	auto& cmdList = m_command.list;

	// Reset allocators and lists for this frame
	allocator->Reset();
	cmdList->Reset(allocator.Get(), nullptr);

	cmdList->OMSetRenderTargets(1, &m_context->getCurrentRenderTargetCDH(), true, &m_depthTexture->getDsvCDH());

	cmdList->RSSetViewports(1, m_context->getViewport());
	cmdList->RSSetScissorRects(1, m_context->getScissorRect());
	m_context->prepareToRender(cmdList.Get());

	// TODO: optimize!
	for (auto& renderCommand : commandQueue) {
		auto& vbuffer = static_cast<DX12VertexBuffer&>(renderCommand.model.mesh->getVertexBuffer());
		vbuffer.init(cmdList.Get());
		if (renderCommand.type != RENDER_COMMAND_TYPE_MODEL) {
			continue;
		}
		auto* tex = static_cast<DX12Texture*>(renderCommand.model.mesh->getMaterial()->getTexture(0));
		if (tex) {
			tex->initBuffers(cmdList.Get());
		}
	}

	cmdList->SetGraphicsRootSignature(m_context->getGlobalRootSignature());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Bind the descriptor heap that will contain all SRVs for this frame
	m_context->getMainGPUDescriptorHeap()->bind(cmdList.Get());
	
	for (auto& command : commandQueue) {
		ShaderPipeline* shaderPipeline = command.model.mesh->getMaterial()->getShader()->getPipeline();
		shaderPipeline->bind(cmdList.Get());

		// Used in most shaders
		shaderPipeline->trySetCBufferVar("sys_mWorld", &glm::transpose(command.transform), sizeof(glm::mat4));
		shaderPipeline->trySetCBufferVar("sys_mView", &camera->getViewMatrix(), sizeof(glm::mat4));
		shaderPipeline->trySetCBufferVar("sys_mProj", &camera->getProjMatrix(), sizeof(glm::mat4));

		command.model.mesh->draw(*this, cmdList.Get());
	}

	// Lastly - transition back buffer to present
	m_context->prepareToPresent(cmdList.Get());
	// Close command list
	cmdList->Close();

	m_context->getDirectQueue()->executeCommandLists({m_command.list.Get()});
}

bool DX12ParticleRenderer::onEvent(const Event& event) {
	return false;
}

void DX12ParticleRenderer::setDepthTexture(DX12RenderableTexture* tex) {
	m_depthTexture = tex;
}
