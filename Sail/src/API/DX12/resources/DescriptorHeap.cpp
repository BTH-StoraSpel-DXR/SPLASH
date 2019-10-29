#include "pch.h"
#include "DescriptorHeap.h"
#include "Sail/Application.h"

DescriptorHeap::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned int numDescriptors, bool shaderVisible, bool frameDependant)
	: m_numDescriptors(numDescriptors)
	, m_index(0) 
	, m_frameDependant(frameDependant)
{
	m_context = Application::getInstance()->getAPI<DX12API>();

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = numDescriptors;
	heapDesc.Flags = (type != D3D12_DESCRIPTOR_HEAP_TYPE_RTV && type != D3D12_DESCRIPTOR_HEAP_TYPE_DSV && shaderVisible) ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.Type = type;
	ThrowIfFailed(m_context->getDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_descHeap)));

	m_incrementSize = m_context->getDevice()->GetDescriptorHandleIncrementSize(type);

	// Store start positions that will be used for the second swap buffer if frameDependant is set to true
	if (frameDependant) {
		m_secondHalfCPUHandleStart = m_descHeap->GetCPUDescriptorHandleForHeapStart();
		m_secondHalfCPUHandleStart.ptr += m_incrementSize * numDescriptors / 2;
		m_secondHalfGPUHandleStart = m_descHeap->GetGPUDescriptorHandleForHeapStart();
		m_secondHalfGPUHandleStart.ptr += m_incrementSize * numDescriptors / 2;

		// Set num descriptors available for each swap buffer
		m_numDescriptors /= 2;
	}
}

DescriptorHeap::~DescriptorHeap() {

}

ID3D12DescriptorHeap* DescriptorHeap::get() const {
	return m_descHeap.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::getNextCPUDescriptorHandle(int nSteps) {
	return getCPUDescriptorHandleForIndex(getAndStepIndex(nSteps));
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::getNextGPUDescriptorHandle() {
	return getGPUDescriptorHandleForIndex(getAndStepIndex());
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::getCurentGPUDescriptorHandle() const {
	return getGPUDescriptorHandleForIndex(m_index);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::getCurentCPUDescriptorHandle() const {
	return getCPUDescriptorHandleForIndex(m_index);
}

unsigned int DescriptorHeap::getDescriptorIncrementSize() const {
	return m_incrementSize;
}

void DescriptorHeap::setIndex(unsigned int index) {
	if (index >= m_numDescriptors) {
		Logger::Error("Tried to set descriptor heap index to a value larger than max (" + std::to_string(m_numDescriptors) + ")!");
	}
	m_index = index;
}

void DescriptorHeap::bind(ID3D12GraphicsCommandList4* cmdList) const {
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_descHeap.Get() };
	cmdList->SetDescriptorHeaps(ARRAYSIZE(descriptorHeaps), descriptorHeaps);
}

unsigned int DescriptorHeap::getAndStepIndex(int nSteps) {
	std::lock_guard<std::mutex> lock(m_getAndStepIndex_mutex);

	unsigned int i = m_index;
	m_index = (m_index + nSteps) % m_numDescriptors;
	// The index should never loop by this method since this means that it has looped mid-frame and will cause the GPU to read out of bounds
	if (m_index < i) {
		Logger::Error("Descriptor heap index has looped mid-frame - this may cause missing textures or GPU crashes! This can be caused by having too many textured objects being rendered simulateously. In that case, consider reducing the amount of textured objects or increase the descriptor heap size (which is currently " + std::to_string(m_numDescriptors) + ")");
	}
	return i;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::getGPUDescriptorHandleForIndex(unsigned int index) const {
	if (index >= m_numDescriptors) {
		Logger::Error("Tried to get out of bounds descriptor heap gpu handle!");
	}
	auto heapHandle = (m_frameDependant && m_context->getSwapIndex() == 1) ? m_secondHalfGPUHandleStart : m_descHeap->GetGPUDescriptorHandleForHeapStart();
	heapHandle.ptr += index * m_incrementSize;
	return heapHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::getCPUDescriptorHandleForIndex(unsigned int index) const {
	if (index >= m_numDescriptors) {
		Logger::Error("Tried to get out of bounds descriptor heap cpu handle!");
	}
	auto heapHandle = (m_frameDependant && m_context->getSwapIndex() == 1) ? m_secondHalfCPUHandleStart : m_descHeap->GetCPUDescriptorHandleForHeapStart();
	heapHandle.ptr += index * m_incrementSize;
	return heapHandle;
}
