#include "pch.h"
#include "DX12IndexBuffer.h"
#include "DX12Utils.h"
#include "Sail/Application.h"

IndexBuffer* IndexBuffer::Create(Mesh::Data& modelData) {
	return SAIL_NEW DX12IndexBuffer(modelData);
}

// TODO: Take in usage (Static or Dynamic) and create a default heap for static only
// TODO: updateData and/or setData
DX12IndexBuffer::DX12IndexBuffer(Mesh::Data& modelData)
	: IndexBuffer(modelData) {
	m_context = Application::getInstance()->getAPI<DX12API>();

	unsigned long* indices = getIndexData(modelData);
	auto numSwapBuffers = m_context->getNumGPUBuffers();

	m_indexBuffers.resize(numSwapBuffers);

	for (unsigned int i = 0; i < numSwapBuffers; i++) {
		m_indexBuffers[i].Attach(DX12Utils::CreateBuffer(m_context->getDevice(), getIndexDataSize(), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, DX12Utils::sUploadHeapProperties));
		m_indexBuffers[i]->SetName(L"Index buffer");

		// Place indices in the buffer
		void* pData;
		D3D12_RANGE readRange{ 0, 0 };
		ThrowIfFailed(m_indexBuffers[i]->Map(0, &readRange, &pData));
		memcpy(pData, indices, getIndexDataSize());
		m_indexBuffers[i]->Unmap(0, nullptr);

	}
	// Delete indices from cpu memory
	Memory::SafeDeleteArr(indices);
}

DX12IndexBuffer::~DX12IndexBuffer() {
	m_context->waitForGPU();
}

void DX12IndexBuffer::bind(void* cmdList) const {
	auto frameIndex = m_context->getSwapIndex();
	auto* dxCmdList = static_cast<ID3D12GraphicsCommandList4*>(cmdList);

	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = m_indexBuffers[frameIndex]->GetGPUVirtualAddress();
	ibView.SizeInBytes = static_cast<UINT>(getIndexDataSize());
	ibView.Format = DXGI_FORMAT_R32_UINT;
	// Later update to just put in a buffer on the renderer to set multiple vertex buffers at once
	dxCmdList->IASetIndexBuffer(&ibView);
}

ID3D12Resource* DX12IndexBuffer::getBuffer() const {
	auto frameIndex = m_context->getSwapIndex();
	return m_indexBuffers[frameIndex].Get();
}
