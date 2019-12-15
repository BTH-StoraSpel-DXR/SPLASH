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

	m_hasBeenInitialized.resize(numSwapBuffers, false);
	m_stillInRAM.resize(numSwapBuffers, true);
	m_uploadIndexBuffers.resize(numSwapBuffers);
	m_defaultIndexBuffers.resize(numSwapBuffers);

	m_byteSize = getIndexDataSize();

	for (unsigned int i = 0; i < numSwapBuffers; i++) {
		m_uploadIndexBuffers[i].Attach(DX12Utils::CreateBuffer(m_context->getDevice(), m_byteSize,
			D3D12_RESOURCE_FLAG_NONE,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			DX12Utils::sUploadHeapProperties));
		m_uploadIndexBuffers[i]->SetName(L"Index buffer upload heap");

		// Place indices in the buffer
		void* pData;
		D3D12_RANGE readRange{ 0, 0 };
		ThrowIfFailed(m_uploadIndexBuffers[i]->Map(0, &readRange, &pData));
		memcpy(pData, indices, m_byteSize);
		m_uploadIndexBuffers[i]->Unmap(0, nullptr);

		// Create the default buffers that the data will be copied to during init()
		m_defaultIndexBuffers[i].Attach(DX12Utils::CreateBuffer(m_context->getDevice(), m_byteSize,
			D3D12_RESOURCE_FLAG_NONE,
			D3D12_RESOURCE_STATE_COPY_DEST, DX12Utils::sDefaultHeapProps));
		m_defaultIndexBuffers[i]->SetName(L"Index buffer default heap");
	}
	// Delete indices from cpu memory
	Memory::SafeDeleteArr(indices);
}

DX12IndexBuffer::~DX12IndexBuffer() {
	m_context->waitForGPU();
}

void DX12IndexBuffer::bind(void* cmdList) {
	auto frameIndex = m_context->getSwapIndex();
	auto* dxCmdList = static_cast<ID3D12GraphicsCommandList4*>(cmdList);
	// Make sure it has been initialized
	init(dxCmdList);

	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = m_defaultIndexBuffers[frameIndex]->GetGPUVirtualAddress();
	ibView.SizeInBytes = static_cast<UINT>(getIndexDataSize());
	ibView.Format = DXGI_FORMAT_R32_UINT;
	// Later update to just put in a buffer on the renderer to set multiple vertex buffers at once
	dxCmdList->IASetIndexBuffer(&ibView);
}

ID3D12Resource* DX12IndexBuffer::getBuffer() const {
	auto frameIndex = m_context->getSwapIndex();
	assert(m_hasBeenInitialized[frameIndex] && "Index buffer has to be initialized before call to getBuffer()");
	return m_defaultIndexBuffers[frameIndex].Get();
}

bool DX12IndexBuffer::init(ID3D12GraphicsCommandList4* cmdList) {
	// This method is called at least once every frame that this texture is used for rendering

	for (unsigned int i = 0; i < m_context->getNumGPUBuffers(); i++) {
		if (m_hasBeenInitialized[i]) {
			// Release the upload heap as soon as the texture has been uploaded to the GPU, but make sure it doesnt happen on the same frame as the upload
			if (m_uploadIndexBuffers[i] && m_initFrameCount != m_context->getFrameCount() && m_queueUsedForUpload->getCompletedFenceValue() > m_initFenceVal) {
				m_uploadIndexBuffers[i].ReleaseAndGetAddressOf();
				m_stillInRAM[i] = false;
			}
			continue;
		}

		// Copy the data from the uploadBuffer to the defaultBuffer
		cmdList->CopyBufferRegion(m_defaultIndexBuffers[i].Get(), 0, m_uploadIndexBuffers[i].Get(), 0, m_byteSize);
		// Transition to usage state
		DX12Utils::SetResourceTransitionBarrier(cmdList, m_defaultIndexBuffers[i].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

		// Signal is only required for the last buffer, since both will be uploaded when the last one is
		if (i == 1) {
			auto type = cmdList->GetType();
			m_queueUsedForUpload = (type == D3D12_COMMAND_LIST_TYPE_DIRECT) ? m_context->getDirectQueue() : m_context->getComputeQueue();
			// Schedule a signal to be called directly after this command list has executed
			// This allows us to know when the data has been uploaded to the GPU
			m_queueUsedForUpload->scheduleSignal([this](UINT64 signaledValue) {
				m_initFenceVal = signaledValue;
			});
			m_initFrameCount = m_context->getFrameCount();
		}

		m_hasBeenInitialized[i] = true;
	}

	return true;
}

unsigned int DX12IndexBuffer::getByteSize() const {
	unsigned int size = 0;

	size += sizeof(*this);

	size += sizeof(bool) * m_stillInRAM.capacity();
	size += sizeof(wComPtr<ID3D12Resource>) * m_uploadIndexBuffers.capacity();
	for (size_t i = 0; i < m_uploadIndexBuffers.size(); i++) {
		if (m_stillInRAM[i]) {
			size += getIndexDataSize();
		}
	}

	size += sizeof(bool) * m_hasBeenInitialized.capacity();
	size += sizeof(wComPtr<ID3D12Resource>) * m_defaultIndexBuffers.capacity();

	return size;
}
