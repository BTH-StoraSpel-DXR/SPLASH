#include "pch.h"
#include "DX12ATexture.h"

#include "Sail/Application.h"

DX12ATexture::DX12ATexture()
	: cpuDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, Application::getInstance()->getAPI<DX12API>()->getNumGPUBuffers() * 3)
	, isRenderableTex(false) 
	, useOneResource(false)
{
	context = Application::getInstance()->getAPI<DX12API>();
	const auto& numSwapBuffers = context->getNumGPUBuffers();

	// States needs to be handled on a per-thread basis
	// They are therefore contained in a 2D vector as states[threadID][frameIndex]

	state.resize(numSwapBuffers);
	srvHeapCDHs.resize(numSwapBuffers);
	depthSrvHeapCDHs.resize(numSwapBuffers);
	uavHeapCDHs.resize(numSwapBuffers);
	textureDefaultBuffers.resize(numSwapBuffers);
	// Store the cpu descriptor handle that will contain the srv for this texture
	for (unsigned int i = 0; i < numSwapBuffers; i++) {
		srvHeapCDHs[i]		= cpuDescHeap.getCPUDescriptorHandleForIndex(i * 3 + 0);
		uavHeapCDHs[i]		= cpuDescHeap.getCPUDescriptorHandleForIndex(i * 3 + 1);
		depthSrvHeapCDHs[i] = cpuDescHeap.getCPUDescriptorHandleForIndex(i * 3 + 2);
	}
}

DX12ATexture::~DX12ATexture() {
	
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12ATexture::getSrvCDH(int swapBuffer) const {
	int i = (swapBuffer == -1) ? context->getSwapIndex() : swapBuffer;
	if (useOneResource) { i = 0; }
	return srvHeapCDHs[i];
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12ATexture::getUavCDH(int swapBuffer) const {
	int i = (swapBuffer == -1) ? context->getSwapIndex() : swapBuffer;
	if (useOneResource) { i = 0; }
	return uavHeapCDHs[i];
}

void DX12ATexture::transitionStateTo(ID3D12GraphicsCommandList4* cmdList, D3D12_RESOURCE_STATES newState, int frameIndex) {
	int i = (frameIndex == -1) ? context->getSwapIndex() : frameIndex;
	auto index = (useOneResource) ? 0 : i;

	if (state[index] == newState) return;

	DX12Utils::SetResourceTransitionBarrier(cmdList, textureDefaultBuffers[index].Get(), state[index], newState);
	state[index] = newState;
}

bool DX12ATexture::isRenderable() const {
	return isRenderableTex;
}

void DX12ATexture::renameBuffer(const std::string& name) const {
	std::wstring stemp = std::wstring(name.begin(), name.end());
	LPCWSTR sw = stemp.c_str();
	for (unsigned int i = 0; i < context->getNumGPUBuffers(); i++) {
		if (textureDefaultBuffers[i]) {
			textureDefaultBuffers[i]->SetName(sw);
		}
	}
}
