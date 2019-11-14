#pragma once
#include "..//BaseComponentSystem.h"

class ID3D12GraphicsCommandList4;

class ParticleSystem final : public BaseComponentSystem {
public:
	ParticleSystem();
	~ParticleSystem();

	void update(float dt);
	void updateOnGPU(ID3D12GraphicsCommandList4* cmdList, const glm::vec3& cameraPos);

	void submitAll() const;
};