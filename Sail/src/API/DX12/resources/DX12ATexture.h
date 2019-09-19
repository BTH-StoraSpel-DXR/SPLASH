#pragma once

#include "../DX12API.h"
#include "DescriptorHeap.h"
#include "../DX12Utils.h"

class DX12ATexture {
public:
	DX12ATexture();
	~DX12ATexture();

	D3D12_CPU_DESCRIPTOR_HANDLE getSrvCDH() const;
	D3D12_CPU_DESCRIPTOR_HANDLE getUavCDH() const;
	//void resetStates(const std::thread::id& resetTo);
	//void setStartState(D3D12_RESOURCE_STATES state);
	void transitionStateTo(ID3D12GraphicsCommandList4* cmdList, D3D12_RESOURCE_STATES newState);
	bool isRenderable() const;
	void renameBuffer(const std::string& name) const;

protected:
	DX12API* context;

	std::vector<wComPtr<ID3D12Resource1>> textureDefaultBuffers;
	DescriptorHeap cpuDescHeap;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> srvHeapCDHs;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> uavHeapCDHs;

	//std::unordered_map<std::thread::id, D3D12_RESOURCE_STATES> states;
	//atomic<D3D12_RESOURCE_STATES> startState;
	std::vector<D3D12_RESOURCE_STATES> state;

	std::mutex m_stateMutex;

	bool isRenderableTex;
	bool useOneResource;
};