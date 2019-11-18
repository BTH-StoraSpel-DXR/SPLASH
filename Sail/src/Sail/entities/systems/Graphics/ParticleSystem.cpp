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
	registerComponent<TransformComponent>(false, true, false);
}

ParticleSystem::~ParticleSystem() {
}

void ParticleSystem::update(float dt) {
	for (auto& e : entities) {
		auto* partComponent = e->getComponent<ParticleEmitterComponent>();

		// Place emitter at entities transform
		if (e->hasComponent<TransformComponent>()) {
			TransformComponent* trans = e->getComponent<TransformComponent>();
			partComponent->position = trans->getMatrixWithoutUpdate() * glm::vec4(partComponent->offset, 1.f);
		}

		glm::vec3 velocityToAdd(0.f);
		if (e->getParent() && e->getParent()->hasComponent<MovementComponent>() && e->hasComponent<CandleComponent>()) {
			if (e->getComponent<CandleComponent>()->isCarried) {
				velocityToAdd = e->getParent()->getComponent<MovementComponent>()->oldVelocity;
			}
		}

		partComponent->velocity = partComponent->constantVelocity + velocityToAdd;

		partComponent->updateTimers(dt);
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
