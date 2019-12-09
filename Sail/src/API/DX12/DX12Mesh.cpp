#include "pch.h"
#include "DX12Mesh.h"
#include "Sail/api/VertexBuffer.h"
#include "Sail/api/IndexBuffer.h"
#include "Sail/Application.h"
#include "Sail/graphics/shader/Shader.h"
#include "resources/DescriptorHeap.h"
#include "shader/DX12ShaderPipeline.h"

Mesh* Mesh::Create(Data& buildData, Shader* shader) {
	return SAIL_NEW DX12Mesh(buildData, shader);
}

Mesh* Mesh::Create(unsigned int numVertices, Shader* shader) {
	return SAIL_NEW DX12Mesh(numVertices, shader);
}

DX12Mesh::DX12Mesh(Data& buildData, Shader* shader)
	: Mesh(buildData, shader) {
	m_context = Application::getInstance()->getAPI<DX12API>();
	material = std::make_shared<PBRMaterial>(shader);
	// Create vertex buffer
	vertexBuffer = std::unique_ptr<VertexBuffer>(VertexBuffer::Create(shader->getPipeline()->getInputLayout(), buildData));
	// Create index buffer if indices are set
	if (buildData.numIndices > 0) {
		indexBuffer = std::unique_ptr<IndexBuffer>(IndexBuffer::Create(buildData));
	}
}

DX12Mesh::DX12Mesh(unsigned int numVertices, Shader* shader)
	: Mesh(numVertices, shader) {
	m_context = Application::getInstance()->getAPI<DX12API>();
	material = std::make_shared<PBRMaterial>(shader);
	// Create vertex buffer
	vertexBuffer = std::unique_ptr<VertexBuffer>(VertexBuffer::Create(shader->getPipeline()->getInputLayout(), numVertices));
}

DX12Mesh::~DX12Mesh() {
}

void DX12Mesh::draw_new(const Renderer& renderer, void* cmdList, int srvOffset) { 
	auto* dxCmdList = static_cast<ID3D12GraphicsCommandList4*>(cmdList);

	bindMaterial(cmdList);
	// Set offset in SRV heap for this mesh 
	dxCmdList->SetGraphicsRootDescriptorTable(m_context->getRootIndexFromRegister("t0"), m_context->getMainGPUDescriptorHeap()->getGPUDescriptorHandleForIndex(m_SRVIndex + srvOffset));
	vertexBuffer->bind(cmdList);

	if (indexBuffer)
		indexBuffer->bind(cmdList);

	// Draw call
	if (indexBuffer)
		dxCmdList->DrawIndexedInstanced(getNumIndices(), 1, 0, 0, 0);
	else
		dxCmdList->DrawInstanced(getNumVertices(), 1, 0, 0);

	static_cast<DX12ShaderPipeline*>(material->getShader()->getPipeline())->instanceFinished();
}

void DX12Mesh::draw(const Renderer& renderer, void* cmdList) {
	draw_new(renderer, cmdList, 0);
}

void DX12Mesh::bindMaterial(void* cmdList) {
	Shader* shader = material->getShader();
	const auto& pbrSettings = material->getPBRSettings();
	DX12ShaderPipeline* pipeline = static_cast<DX12ShaderPipeline*>(shader->getPipeline());
	pipeline->trySetCBufferVar("sys_material_pbr", (void*)& pbrSettings, sizeof(PBRMaterial::PBRSettings));
	m_SRVIndex = pipeline->setMaterial(material.get(), cmdList);
}

unsigned int DX12Mesh::getSRVIndex() {
	return m_SRVIndex;
}
