#include "pch.h"
#include "DX12VertexBuffer.h"
#include "Sail/Application.h"
#include "DX12Utils.h"

VertexBuffer* VertexBuffer::Create(const InputLayout& inputLayout, Mesh::Data& modelData) {
	return SAIL_NEW DX12VertexBuffer(inputLayout, modelData);
}

// TODO: Take in usage (Static or Dynamic) and create a default heap for static only
// TODO: updateData and/or setData
DX12VertexBuffer::DX12VertexBuffer(const InputLayout& inputLayout, Mesh::Data& modelData)
	: VertexBuffer(inputLayout, modelData) {
	DX12API* context = Application::getInstance()->getAPI<DX12API>();
	void* vertices = getVertexData(modelData);

	m_vertexBuffer.Attach(DX12Utils::CreateBuffer(context->getDevice(), getVertexDataSize(), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, DX12Utils::sUploadHeapProperties));
	m_vertexBuffer->SetName(L"Vertex buffer");

	// Place verticies in the buffer
	void* pData;
	D3D12_RANGE readRange{ 0, 0 };
	ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, &pData));
	memcpy(pData, vertices, getVertexDataSize());
	m_vertexBuffer->Unmap(0, nullptr);

	// Delete vertices from cpu memory
	free(vertices);
}

DX12VertexBuffer::~DX12VertexBuffer() {

}

void DX12VertexBuffer::bind(void* cmdList) const {
	auto* dxCmdList = static_cast<ID3D12GraphicsCommandList4*>(cmdList);

	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	vbView.SizeInBytes = static_cast<UINT>(getVertexDataSize());
	vbView.StrideInBytes = static_cast<UINT>(getVertexDataStride());
	// Later update to just put in a buffer on the renderer to set multiple vertex buffers at once
	dxCmdList->IASetVertexBuffers(0, 1, &vbView);
}

ID3D12Resource1* DX12VertexBuffer::getBuffer() const {
	return m_vertexBuffer.Get();
}
