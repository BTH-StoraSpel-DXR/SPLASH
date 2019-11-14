#include "pch.h"
#include "ParticleSystem.h"
#include "Sail/Application.h"
#include "Sail/entities/components/Components.h"
#include "Sail/entities/Entity.h"
#include "API/DX12/DX12VertexBuffer.h"
#include "API/DX12/shader/DX12ConstantBuffer.h"
#include "API/DX12/shader/DX12StructuredBuffer.h"
#include "API/DX12/resources/DescriptorHeap.h"
#include "API/DX12/DX12Utils.h"
#include "Sail/utils/Timer.h"

#include "Sail/graphics/shader/compute/ParticleComputeShader.h"
#include "Sail/graphics/shader/dxr/GBufferOutShader.h"

ParticleSystem::ParticleSystem() {
	registerComponent<ParticleEmitterComponent>(true, true, true);

	
}

ParticleSystem::~ParticleSystem() {
}

void ParticleSystem::update(float dt) {
	for (auto& e : entities) {
		e->getComponent<ParticleEmitterComponent>()->updateTimers(dt);
	}
}

void ParticleSystem::updateOnGPU(ID3D12GraphicsCommandList4* cmdList, const glm::vec3& cameraPos) {
	for (auto& e : entities) {
		e->getComponent<ParticleEmitterComponent>()->updateOnGPU(cmdList, cameraPos);
	}
}

void ParticleSystem::submitAll() const {
	for (auto& e : entities) {
		e->getComponent<ParticleEmitterComponent>()->submit();
	}
}
