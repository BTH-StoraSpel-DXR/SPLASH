#include "pch.h"
#include "DX12ATexture.h"

#include "Sail/Application.h"

DX12ATexture::DX12ATexture()
	: cpuDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, Application::getInstance()->getAPI<DX12API>()->getNumSwapBuffers() * 2)
	, isRenderableTex(false) 
	, useOneResource(false)
{
	context = Application::getInstance()->getAPI<DX12API>();
	const auto& numSwapBuffers = context->getNumSwapBuffers();

	// States needs to be handled on a per-thread basis
	// They are therefore contained in a 2D vector as states[threadID][frameIndex]

	state.resize(numSwapBuffers);
	srvHeapCDHs.resize(numSwapBuffers);
	uavHeapCDHs.resize(numSwapBuffers);
	textureDefaultBuffers.resize(numSwapBuffers);
	// Store the cpu descriptor handle that will contain the srv for this texture
	for (unsigned int i = 0; i < numSwapBuffers; i++) {
		srvHeapCDHs[i] = cpuDescHeap.getCPUDescriptorHandleForIndex(i * 2 + 0);
		uavHeapCDHs[i] = cpuDescHeap.getCPUDescriptorHandleForIndex(i * 2 + 1);
	}
}

DX12ATexture::~DX12ATexture() {
	
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12ATexture::getSrvCDH() const {
	if (useOneResource) { return srvHeapCDHs[0]; }
	return srvHeapCDHs[context->getFrameIndex()];
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12ATexture::getUavCDH() const {
	if (useOneResource) { return uavHeapCDHs[0]; }
	return uavHeapCDHs[context->getFrameIndex()];
}

//void DX12ATexture::resetStates(const std::thread::id& resetTo) {
//	// Assume thread 0 contains the correct state
//	//if (states.find(resetTo) != states.end()) {
//		//startState = states[resetTo][context->getFrameIndex()];
//	if (!states.empty()) {
//		startState = (*states.begin()).second;
//	}
//	//}
//
//	states.clear();
//	/*for (unsigned int threadID = 0; threadID < numThreads; threadID++) {
//		for (unsigned int frameIndex = 0; frameIndex < context->getNumSwapBuffers(); frameIndex++) {
//			states[threadID][frameIndex] = currentState;
//		}
//	}*/
//}

void DX12ATexture::transitionStateTo(ID3D12GraphicsCommandList4* cmdList, D3D12_RESOURCE_STATES newState) {
	unsigned int frameIndex = context->getFrameIndex();
	auto index = (useOneResource) ? 0 : frameIndex;

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
	for (unsigned int i = 0; i < context->getNumSwapBuffers(); i++) {
		if (textureDefaultBuffers[i]) {
			textureDefaultBuffers[i]->SetName(sw);
		}
	}
}
