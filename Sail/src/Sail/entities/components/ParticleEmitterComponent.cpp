#include "pch.h"
#include "ParticleEmitterComponent.h"

#include "Sail/Application.h"
#include "Sail/graphics/shader/compute/ParticleComputeShader.h"
#include "API/DX12/shader/DX12ConstantBuffer.h"
#include "API/DX12/DX12Utils.h"
#include "API/DX12/DX12VertexBuffer.h"
#include "API/DX12/resources/DescriptorHeap.h"

ParticleEmitterComponent::ParticleEmitterComponent()
	: m_hasBeenCreatedInSystem(false)
{

	size = 0.2f;
	offset = { 0.f, 0.f, 0.f };
	position = { 0.f, 0.f, 0.f };
	spread = { 0.f, 0.f, 0.f };
	constantVelocity = { 0.f, 0.f, 0.f };
	velocity = { 0.f, 0.f, 0.f };
	acceleration = { 0.f, 0.f, 0.f };
	lifeTime = 1.0f;
	spawnRate = 0.1f;
	spawnTimer = 0.0f;
	atlasSize = glm::uvec2(1U, 1U);
	drag = 0.0f;
	maxNumberOfParticles = 300;


	init();
}

ParticleEmitterComponent::~ParticleEmitterComponent() {
	delete[] m_cpuOutput;
	delete[] m_particleLife;
}

void ParticleEmitterComponent::init() {
	m_particleLife = SAIL_NEW std::vector<float>[DX12API::NUM_GPU_BUFFERS];
	m_cpuOutput = SAIL_NEW CPUOutput[DX12API::NUM_GPU_BUFFERS];
	for (unsigned int i = 0; i < DX12API::NUM_GPU_BUFFERS; i++) {
		m_cpuOutput[i].previousNrOfParticles = 0;
		m_cpuOutput[i].lastFrameTime = 0;
	}

	m_timer.startTimer();
	m_startTime = m_timer.getStartTime();
	m_gpuUpdates = 0;
}

void ParticleEmitterComponent::syncWithGPUUpdate(unsigned int swapBufferIndex, unsigned int outputVertexBufferSize) {
	// Remove
	unsigned int numToRemove = (unsigned int)m_cpuOutput[swapBufferIndex].toRemove.size();

	for (unsigned int i = 0; i < numToRemove; i++) {
		unsigned int swapIndex = 0;
		if (m_cpuOutput[swapBufferIndex].toRemove[i] < m_cpuOutput[swapBufferIndex].previousNrOfParticles - numToRemove) {
			swapIndex = m_cpuOutput[swapBufferIndex].previousNrOfParticles - i - 1;
			int counter = 0;

			while (m_cpuOutput[swapBufferIndex].toRemove[numToRemove - counter - 1] >= swapIndex && counter < numToRemove - 1) {
				swapIndex--;
				counter++;
			}

			if (swapIndex > m_cpuOutput[swapBufferIndex].toRemove[i]) {
				m_particleLife[swapBufferIndex][m_cpuOutput[swapBufferIndex].toRemove[i]] = m_particleLife[swapBufferIndex][swapIndex];
			}
		}
	}

	m_particleLife[swapBufferIndex].erase(m_particleLife[swapBufferIndex].begin() + m_cpuOutput[swapBufferIndex].previousNrOfParticles - numToRemove, m_particleLife[swapBufferIndex].end());

	// Add
	unsigned int numToAdd = glm::min(glm::min((unsigned int)m_cpuOutput[swapBufferIndex].newParticles.size(), m_maxParticlesSpawnPerFrame), (unsigned int)(floor(outputVertexBufferSize / 6) - (m_cpuOutput[swapBufferIndex].previousNrOfParticles - numToRemove)));
	for (unsigned int i = 0; i < numToAdd; i++) {
		m_particleLife[swapBufferIndex].emplace_back();
		m_particleLife[swapBufferIndex].back() = lifeTime - (m_cpuOutput[swapBufferIndex].lastFrameTime - m_cpuOutput[swapBufferIndex].newParticles[i].spawnTime);
	}
}

bool ParticleEmitterComponent::hasBeenCreatedInSystem() {
	return m_hasBeenCreatedInSystem;
}

void ParticleEmitterComponent::setAsCreatedInSystem() { 
	m_hasBeenCreatedInSystem = true;
}

const std::string& ParticleEmitterComponent::getTextureName() const {
	return m_textureName;
}

void ParticleEmitterComponent::spawnParticles(int particlesToSpawn) {
	int maxCount = 0;

	for (unsigned int i = 0; i < DX12API::NUM_GPU_BUFFERS; i++) {
		if ((int) m_cpuOutput->newParticles.size() > maxCount) {
			maxCount = m_cpuOutput->newParticles.size();
		}
	}

	particlesToSpawn = glm::min(particlesToSpawn, (int) (m_maxParticlesSpawnPerFrame - maxCount));

	//Spawn particles for all swap buffers
	for (int j = 0; j < particlesToSpawn; j++) {
		//Get random spread
		glm::vec3 randVec = { (Utils::rnd() - 0.5f) * 2.0f, (Utils::rnd() - 0.5f) * 2.0f, (Utils::rnd() - 0.5f) * 2.0f };

		//Get time
		float time = m_timer.getTimeSince<float>(m_startTime);

		//Add particle to spawn list
		for (unsigned int i = 0; i < DX12API::NUM_GPU_BUFFERS; i++) {
			m_cpuOutput[i].newParticles.emplace_back();
			m_cpuOutput[i].newParticles.back().pos = position + (randVec + glm::vec3(0.0f, 1.0f, 0.0f)) * 0.02f ;
			m_cpuOutput[i].newParticles.back().spread = spread * randVec;
			m_cpuOutput[i].newParticles.back().spawnTime = time;
		}
	}
}

void ParticleEmitterComponent::updateTimers(float dt) {
	for (int i = 0; i < DX12API::NUM_GPU_BUFFERS; i++) {
		for (int j = 0; j < m_particleLife[i].size(); j++) {
			m_particleLife[i][j] -= dt;
		}
	}

	if (spawnTimer >= spawnRate) {
		//Spawn the correct number of particles
		int particlesToSpawn = (int)glm::floor(spawnTimer / glm::max(spawnRate, 0.0001f));
		spawnParticles(particlesToSpawn);
		//Decrease timer
		spawnTimer -= glm::max(spawnRate, 0.0001f) * particlesToSpawn;
	}
	spawnTimer += dt;
}

void ParticleEmitterComponent::updateOnGPU(ID3D12GraphicsCommandList4* cmdList, const glm::vec3& cameraPos, EmitterData& data, ComputeShaderDispatcher& dispatcher) {
	auto* context = Application::getInstance()->getAPI<DX12API>();

	if (m_gpuUpdates < (int)(DX12API::NUM_GPU_BUFFERS * 2)) {
		//Initialize timers the first two times the buffers are ran
		float elapsedTime = m_timer.getTimeSince<float>(m_startTime) - m_cpuOutput[context->getSwapIndex()].lastFrameTime;
		m_cpuOutput[context->getSwapIndex()].lastFrameTime += elapsedTime;
		m_gpuUpdates++;
	} else {
		data.outputVertexBuffer->init(cmdList);


		auto* settings = data.particleShader->getComputeSettings();

		//----Compute shader constant buffer----
		m_inputData.cameraPos = cameraPos;
		m_inputData.previousNrOfParticles = m_cpuOutput[context->getSwapIndex()].previousNrOfParticles;
		m_inputData.maxOutputVertices = data.outputVertexBufferSize;
		float elapsedTime = m_timer.getTimeSince<float>(m_startTime) - m_cpuOutput[context->getSwapIndex()].lastFrameTime;
		m_inputData.frameTime = elapsedTime;
		m_inputData.size = size;
		m_inputData.atlasSize = atlasSize;
		m_inputData.drag = drag;

		//Update timer for this buffer
		m_cpuOutput[context->getSwapIndex()].lastFrameTime += elapsedTime;

		//Gather particles to remove
		for (unsigned int i = 0; i < m_particleLife[context->getSwapIndex()].size(); i++) {
			if (m_particleLife[context->getSwapIndex()][i] < 0.0f && m_cpuOutput[context->getSwapIndex()].toRemove.size() < m_maxParticlesSpawnPerFrame - 1) {
				m_cpuOutput[context->getSwapIndex()].toRemove.emplace_back();
				m_cpuOutput[context->getSwapIndex()].toRemove.back() = i;
			}
		}

		//Sort it so that the overlaps when swaping on the gpu can easily be found
		std::sort(m_cpuOutput[context->getSwapIndex()].toRemove.begin(), m_cpuOutput[context->getSwapIndex()].toRemove.end());

		// Particles to remove
		unsigned int numPartRem = (unsigned int)m_cpuOutput[context->getSwapIndex()].toRemove.size();
		m_inputData.numParticlesToRemove = numPartRem;
		for (unsigned int i = 0; i < numPartRem; i++) {
			m_inputData.particlesToRemove[i] = m_cpuOutput[context->getSwapIndex()].toRemove[i];
		}

		// Particles to add
		unsigned int numPart = glm::min(glm::min((unsigned int)m_cpuOutput[context->getSwapIndex()].newParticles.size(), m_maxParticlesSpawnPerFrame), (unsigned int)(floor(data.outputVertexBufferSize / 6) - (m_cpuOutput[context->getSwapIndex()].previousNrOfParticles - numPartRem)));
		m_inputData.numParticles = numPart;
		for (unsigned int i = 0; i < numPart; i++) {
			const NewParticleInfo* newParticle_i = &m_cpuOutput[context->getSwapIndex()].newParticles[i];
			m_inputData.particles[i].position = newParticle_i->pos;
			m_inputData.particles[i].velocity = velocity + newParticle_i->spread;
			m_inputData.particles[i].acceleration = acceleration;
			m_inputData.particles[i].spawnTime = m_cpuOutput[context->getSwapIndex()].lastFrameTime - newParticle_i->spawnTime;
		}

		data.inputConstantBuffer->updateData_new(&m_inputData, sizeof(ComputeInput), 0);
		data.inputConstantBuffer->bind_new(cmdList, 0, true);
		//--------------------------------------

		auto& cdh = context->getComputeGPUDescriptorHeap()->getCurentCPUDescriptorHandle();
		cdh.ptr += context->getComputeGPUDescriptorHeap()->getDescriptorIncrementSize() * 10;

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = data.outputVertexBufferSize;
		uavDesc.Buffer.StructureByteStride = 4 * 14; // TODO: replace with sizeof(Vertex)
		context->getDevice()->CreateUnorderedAccessView(data.outputVertexBuffer->getBuffer(), nullptr, &uavDesc, cdh);
		// Transition to UAV access
		DX12Utils::SetResourceTransitionBarrier(cmdList, data.outputVertexBuffer->getBuffer(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		//----Compute shader physics buffer----
		cdh.ptr += context->getComputeGPUDescriptorHeap()->getDescriptorIncrementSize();
		uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = data.outputVertexBufferSize / 6;
		uavDesc.Buffer.StructureByteStride = data.particlePhysicsSize;
		context->getDevice()->CreateUnorderedAccessView(data.physicsBufferDefaultHeap[context->getSwapIndex()].Get(), nullptr, &uavDesc, cdh);
		//-------------------------------------

		dispatcher.dispatch(*data.particleShader, Shader::ComputeShaderInput(), cmdList);

		context->getComputeGPUDescriptorHeap()->getAndStepIndex(12);

		// Transition to Cbuffer usage
		DX12Utils::SetResourceTransitionBarrier(cmdList, data.outputVertexBuffer->getBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		//Sync cpu arrays with GPU
		syncWithGPUUpdate(context->getSwapIndex(), data.outputVertexBufferSize);

		// Update nr of particles in this buffer and erase added and removed emitters from the queues
		m_cpuOutput[context->getSwapIndex()].previousNrOfParticles = glm::min(m_cpuOutput[context->getSwapIndex()].previousNrOfParticles + numPart - numPartRem, data.outputVertexBufferSize / 6);

		m_cpuOutput[context->getSwapIndex()].newParticles.clear();
		m_cpuOutput[context->getSwapIndex()].toRemove.clear();
	}
}

void ParticleEmitterComponent::setTexture(const std::string& textureName) {
	m_textureName = textureName;
}