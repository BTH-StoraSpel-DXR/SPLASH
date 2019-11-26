#include "pch.h"
#include "ParticleSystem.h"
#include "Sail/Application.h"
#include "Sail/entities/components/Components.h"
#include "Sail/entities/Entity.h"
#include "Sail/graphics/shader/dxr/GBufferOutShader.h"
#include "Sail/graphics/shader/dxr/GBufferOutShaderNoDepth.h"
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
	registerComponent<RenderInActiveGameComponent>(true, false, false);

	m_dispatcher = std::unique_ptr<ComputeShaderDispatcher>(ComputeShaderDispatcher::Create());
}

ParticleSystem::~ParticleSystem() {
	stop();
}

void ParticleSystem::update(float dt) {
	for (auto& e : entities) {
		auto* partComponent = e->getComponent<ParticleEmitterComponent>();

		if (!partComponent->hasBeenCreatedInSystem()) {
			initEmitter(e, partComponent);
		}

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
	// Mark removed emitters as dead
	for (auto& it : m_emitters) {
		if (!it.first->hasComponent<ParticleEmitterComponent>()) {
			it.second.isDead = true;
		}
	}
	// Remove from m_emitters those which are dead and is not in use by any renderers/not used for 2 frames
	for (auto& it = std::begin(m_emitters); it != std::end(m_emitters);) {
		if (it->second.framesDead >= 2) {
			delete[] it->second.physicsBufferDefaultHeap;
			it = m_emitters.erase(it);
		} else {
			++it;
		}
	}
}

void ParticleSystem::updateOnGPU(ID3D12GraphicsCommandList4* cmdList, const glm::vec3& cameraPos) {
	// Increase dead frame count on dead emitters
	for (auto& it : m_emitters) {
		auto& emitterData = it.second;
		if (emitterData.isDead) {
			emitterData.framesDead++;
		}
	}

	m_dispatcher->begin(cmdList);
	for (auto& e : entities) {
		auto* emitterComp = e->getComponent<ParticleEmitterComponent>();

		if (!emitterComp || !emitterComp->hasBeenCreatedInSystem()) {
			continue;
		}

		auto& emitterData = m_emitters.at(e);
		emitterData.model->getMesh(0)->getMaterial()->setAlbedoTexture(emitterComp->getTextureName());

		emitterComp->updateOnGPU(cmdList, cameraPos, emitterData, *m_dispatcher);
	}
}

void ParticleSystem::submitAll() const {
	Renderer* renderer = Application::getInstance()->getRenderWrapper()->getParticleRenderer();
	Renderer::RenderFlag flags = Renderer::MESH_DYNAMIC;
	flags |= Renderer::IS_VISIBLE_ON_SCREEN;
	for (auto& e : entities) {
		auto* emitterComp = e->getComponent<ParticleEmitterComponent>();

		if (!emitterComp || !emitterComp->hasBeenCreatedInSystem()) {
			continue;
		}

		renderer->submit(
			m_emitters.at(e).model.get(),
			glm::identity<glm::mat4>(),
			flags,
			1
		);
	}
}

void ParticleSystem::initEmitter(Entity* owner, ParticleEmitterComponent* component) {
	ParticleEmitterComponent::EmitterData& emitter = m_emitters.insert({owner, ParticleEmitterComponent::EmitterData()}).first->second;
	
	emitter.particleShader = std::make_unique<ParticleComputeShader>();
	auto& gbufferShader = Application::getInstance()->getResourceManager().getShaderSet<GBufferOutShader>();
	auto& inputLayout = gbufferShader.getPipeline()->getInputLayout();

	emitter.outputVertexBufferSize = 6 * 1700;
	auto& noDepthShader = Application::getInstance()->getResourceManager().getShaderSet<GBufferOutShaderNoDepth>();
	emitter.model = std::make_unique<Model>(emitter.outputVertexBufferSize, &noDepthShader);

	emitter.outputVertexBuffer = static_cast<DX12VertexBuffer*>(&emitter.model->getMesh(0)->getVertexBuffer());

	auto* context = Application::getInstance()->getAPI<DX12API>();

	emitter.particlePhysicsSize = 9 * 4; // 9 floats times 4 bytes

	emitter.physicsBufferDefaultHeap = SAIL_NEW wComPtr<ID3D12Resource>[DX12API::NUM_GPU_BUFFERS];
	for (UINT i = 0; i < DX12API::NUM_GPU_BUFFERS; i++) {
		emitter.physicsBufferDefaultHeap[i].Attach(DX12Utils::CreateBuffer(context->getDevice(), emitter.outputVertexBufferSize / 6 * emitter.particlePhysicsSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DX12Utils::sDefaultHeapProps));
		emitter.physicsBufferDefaultHeap[i]->SetName(L"Particle Physics Default Resource Heap");
	}

	component->setAsCreatedInSystem();
}

void ParticleSystem::stop() {
	//Clean each after game
	for (auto& it : m_emitters) {
		auto& emitter = it.second;
		delete[] emitter.physicsBufferDefaultHeap;
	}

	m_emitters.clear();
}
