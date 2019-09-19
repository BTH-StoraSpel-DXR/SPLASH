#pragma once

#include "../DX12API.h"
#include <mutex>

class DescriptorHeap {
public:
	DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned int numDescriptors, bool shaderVisible = false);
	~DescriptorHeap();

	// An internal index behaving like a ring buffer is used
	D3D12_CPU_DESCRIPTOR_HANDLE getNextCPUDescriptorHandle(int nSteps = 1);
	D3D12_CPU_DESCRIPTOR_HANDLE getCPUDescriptorHandleForIndex(unsigned int index) const;

	// An internal index behaving like a ring buffer is used
	D3D12_GPU_DESCRIPTOR_HANDLE getNextGPUDescriptorHandle();
	D3D12_GPU_DESCRIPTOR_HANDLE getGPUDescriptorHandleForIndex(unsigned int index) const;

	D3D12_GPU_DESCRIPTOR_HANDLE	getCurentGPUDescriptorHandle() const;
	unsigned int getDescriptorIncrementSize() const;

	void setIndex(unsigned int index);

	void bind(ID3D12GraphicsCommandList4* cmdList) const;
	unsigned int getAndStepIndex(int nSteps = 1);

private:
	std::mutex m_getAndStepIndex_mutex;

private:
	wComPtr<ID3D12DescriptorHeap> m_descHeap;
	unsigned int m_incrementSize;
	unsigned int m_numDescriptors;

	unsigned int m_index;

};