#include "pch.h"
#include "DX12StructuredBuffer.h"
#include "Sail/Application.h"
#include "../DX12Utils.h"

namespace ShaderComponent {

	StructuredBuffer* StructuredBuffer::Create(void* initData, unsigned int size, unsigned int numElements, unsigned int stride, BIND_SHADER bindShader, unsigned int slot) {
		return SAIL_NEW DX12StructuredBuffer(initData, size, numElements, stride, bindShader, slot);
	}

	// This constructor is not used by ShaderPipeline
	DX12StructuredBuffer::DX12StructuredBuffer(void* initData, unsigned int numElements, unsigned int stride)
	: m_register(-1)
	, m_stride(stride)
	, m_numElements(numElements)
	, m_elementByteSize(stride)
	{
		m_context = Application::getInstance()->getAPI<DX12API>();
		auto numSwapBuffers = m_context->getNumGPUBuffers();

		m_srvHeap = std::make_unique<DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, numSwapBuffers);

		m_bufferUploadHeap.resize(numSwapBuffers);
		m_cbGPUAddress.resize(numSwapBuffers);
		m_srvCDHs.resize(numSwapBuffers);
		m_resourceHeapSize.resize(numSwapBuffers);
		for (UINT i = 0; i < numSwapBuffers; i++) {
			m_resourceHeapSize[i] = stride * numElements;
			// Store srv handles
			m_srvCDHs[i] = m_srvHeap->getCPUDescriptorHandleForIndex(i);
		}

		createBuffers(numElements);
		for (UINT i = 0; i < numSwapBuffers; i++) {
			// Place initData in the buffer
			memcpy(m_cbGPUAddress[i], initData, stride * numElements);
		}
	}

	DX12StructuredBuffer::DX12StructuredBuffer(void* initData, unsigned int size, unsigned int numElements, unsigned int stride, BIND_SHADER bindShader, unsigned int slot)
		: m_register(slot)
		, m_stride(stride)
		, m_numElements(numElements)
		, m_elementByteSize(size)
	{
		m_context = Application::getInstance()->getAPI<DX12API>();
		auto numSwapBuffers = m_context->getNumGPUBuffers();

		m_srvHeap = std::make_unique<DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, numSwapBuffers);

		m_bufferUploadHeap.resize(numSwapBuffers);
		m_cbGPUAddress.resize(numSwapBuffers);
		m_srvCDHs.resize(numSwapBuffers);
		m_resourceHeapSize.resize(numSwapBuffers);
		for (UINT i = 0; i < numSwapBuffers; i++) {
			m_resourceHeapSize[i] = size * MAX_ELEMENTS * MAX_MESHES_PER_FRAME;
			// Store srv handles
			m_srvCDHs[i] = m_srvHeap->getCPUDescriptorHandleForIndex(i);
		}
		
		createBuffers(numElements);
		for (UINT i = 0; i < numSwapBuffers; i++) {
			// Place initData in the buffer
			memcpy(m_cbGPUAddress[i], initData, size);
		}
	}

	DX12StructuredBuffer::~DX12StructuredBuffer() {
	}

	void DX12StructuredBuffer::updateData(const void* newData, unsigned int numElements, int meshIndex, unsigned int offset) {
		// Line below used to check animated models, currently uncommented since StructuredBuffers are used for multiple things
		//assert(numElements < MAX_ELEMENTS && "Too many elements! Increase MAX_ELEMENTS in DX12StrucutedBuffer.h");
		assert(meshIndex < MAX_MESHES_PER_FRAME && "Too many meshes! Increase MAX_MESHES_PER_FRAME in DX12StrucutedBuffer.h");

		// This method needs to be run every frame to make sure the buffer for all framebuffers are kept updated
		auto frameIndex = m_context->getSwapIndex();
		auto numSwapBuffers = m_context->getNumGPUBuffers();

		memcpy(m_cbGPUAddress[frameIndex] + MAX_ELEMENTS * m_elementByteSize * meshIndex + offset, newData, m_elementByteSize * numElements);
		m_numElements = numElements;
	}

	void DX12StructuredBuffer::bind(void* cmdList) const {
		auto* dxCmdList = static_cast<ID3D12GraphicsCommandList4*>(cmdList);
		auto frameIndex = m_context->getSwapIndex();

		UINT rootIndex = m_context->getRootIndexFromRegister("t" + std::to_string(m_register));

		// Copy SRV and bind
		m_context->getDevice()->CopyDescriptorsSimple(1, m_context->getComputeGPUDescriptorHeap()->getNextCPUDescriptorHandle(), m_srvCDHs[frameIndex], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	void DX12StructuredBuffer::bind_new(void* cmdList, int meshIndex) const {
		auto* dxCmdList = static_cast<ID3D12GraphicsCommandList4*>(cmdList);
		auto frameIndex = m_context->getSwapIndex();

		UINT rootIndex = m_context->getRootIndexFromRegister("t" + std::to_string(m_register));

		// Create SRV with correct starting index
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = MAX_ELEMENTS * meshIndex;
		srvDesc.Buffer.NumElements = MAX_ELEMENTS;
		srvDesc.Buffer.StructureByteStride = m_stride;
		m_context->getDevice()->CreateShaderResourceView(m_bufferUploadHeap[frameIndex].Get(), &srvDesc, m_context->getComputeGPUDescriptorHeap()->getNextCPUDescriptorHandle());
	}

	ID3D12Resource* DX12StructuredBuffer::getBuffer() const {
		return m_bufferUploadHeap[m_context->getSwapIndex()].Get();
	}

	void DX12StructuredBuffer::createBuffers(unsigned int numElements) {
		auto frameIndex = m_context->getSwapIndex();
		auto numSwapBuffers = m_context->getNumGPUBuffers();
		static_cast<DX12API*>(Application::getInstance()->getAPI())->waitForGPU();
		// Create an upload heap to hold the constant buffer
		// create a resource heap, and pointer to cbv for each frame
		for (UINT i = 0; i < numSwapBuffers; i++) {

			m_bufferUploadHeap[i].Attach(DX12Utils::CreateBuffer(m_context->getDevice(), m_resourceHeapSize[frameIndex], D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, DX12Utils::sUploadHeapProperties));
			m_bufferUploadHeap[i]->SetName(L"Constant Buffer Upload Resource Heap");

			// Map the constant buffer and keep it mapped for the duration of its lifetime
			D3D12_RANGE readRange{ 0, 0 }; // We do not intend to read from this resource on the CPU. (End is less than or equal to begin)
			ThrowIfFailed(m_bufferUploadHeap[i]->Map(0, &readRange, reinterpret_cast<void**>(&m_cbGPUAddress[i])));

		}

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = numElements;
		srvDesc.Buffer.StructureByteStride = m_stride;
		for (UINT i = 0; i < numSwapBuffers; i++) {
			// Create SRV
			m_context->getDevice()->CreateShaderResourceView(m_bufferUploadHeap[i].Get(), &srvDesc, m_srvCDHs[i]);
		}

	}

}
