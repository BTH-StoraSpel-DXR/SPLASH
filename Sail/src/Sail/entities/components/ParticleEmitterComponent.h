#pragma once

#include "Component.h"

#include <glm/glm.hpp>

#include "Sail/api/ComputeShaderDispatcher.h"
#include "API/DX12/DX12API.h"
#include "Sail/utils/Timer.h"

class DX12VertexBuffer;
namespace ShaderComponent {
	class DX12ConstantBuffer;
};
class ParticleComputeShader;
struct ID3D12GraphicsCommandList4;

class ParticleEmitterComponent : public Component<ParticleEmitterComponent> {
public:
	friend class ParticleSystem;

	ParticleEmitterComponent();
	~ParticleEmitterComponent();

	float size;
	glm::vec3 offset;
	glm::vec3 position;
	glm::vec3 spread;
	glm::vec3 constantVelocity;
	glm::vec3 velocity;
	glm::vec3 acceleration;
	glm::uvec2 atlasSize;
	float drag;
	float lifeTime;
	float spawnRate;
	float spawnTimer;
	int maxNumberOfParticles;
	bool isActive;


#ifdef DEVELOPMENT
	const unsigned int getByteSize() const override {
		/* TODO: Fix component size */
		unsigned int size = sizeof(*this);
		size += sizeof(CPUOutput);
		size += sizeof(NewParticleInfo) * m_cpuOutput->newParticles.size();
		size += sizeof(unsigned int) * m_cpuOutput->toRemove.size();
		size += sizeof(float) * m_particleLife->size();
		//size += m_model->getByteSize(); // Stored in ParticleSystem
		return size;
	}
#endif

	struct EmitterData {
		ParticleComputeShader* particleShader;

		DX12VertexBuffer* outputVertexBuffer;
		unsigned int outputVertexBufferSize;

		wComPtr<ID3D12Resource>* physicsBufferDefaultHeap;
		int particlePhysicsSize;

		std::unique_ptr<ShaderComponent::DX12ConstantBuffer> inputConstantBuffer;
		unsigned int inputConstantBufferSize;

		std::unique_ptr<Model> model;
		bool isDead = false;
		unsigned int framesDead = 0;
	};

private:
	void init();
	void syncWithGPUUpdate(unsigned int swapBufferIndex, unsigned int outputVertexBufferSize);

	// Used in ParticleSystem
	bool hasBeenCreatedInSystem();
	void setAsCreatedInSystem(bool created);
	const std::string& getTextureName() const;

public:
	void spawnParticles(int particlesToSpawn);

	void updateTimers(float dt);
	void updateOnGPU(ID3D12GraphicsCommandList4* cmdList, const glm::vec3& cameraPos, EmitterData& data, ComputeShaderDispatcher& dispatcher);

	void setTexture(const std::string& textureName);

private:
	bool m_hasBeenCreatedInSystem;

	Timer m_timer;
	INT64 m_startTime;

	int m_gpuUpdates;

	std::vector<float>* m_particleLife;
	std::string m_textureName;

	struct NewParticleInfo {
		glm::vec3 pos;
		glm::vec3 spread;
		float spawnTime;
	};

	struct CPUOutput {
		std::vector<NewParticleInfo> newParticles;
		std::vector<unsigned int> toRemove;
		unsigned int previousNrOfParticles;
		float lastFrameTime;
	};

	CPUOutput* m_cpuOutput;

	struct ParticleData {
		glm::vec3 position;
		float padding0;
		glm::vec3 velocity;
		float padding1;
		glm::vec3 acceleration;
		float spawnTime;
	};

	static const unsigned int m_maxParticlesSpawnPerFrame = 312;

	struct ComputeInput {
		ParticleData particles[m_maxParticlesSpawnPerFrame];
		unsigned int particlesToRemove[m_maxParticlesSpawnPerFrame];
		glm::vec3 cameraPos;
		unsigned int numParticles;
		unsigned int numParticlesToRemove;
		unsigned int previousNrOfParticles;
		unsigned int maxOutputVertices;
		float frameTime;
		float size;
		glm::uvec2 atlasSize;
		float drag;
	};

	ComputeInput m_inputData;
};