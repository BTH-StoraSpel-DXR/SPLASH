struct Particle{
	float3 position;
	float padding0;
	float3 velocity;
	float padding1;
	float3 acceleration;
	float spawnTime;
};

struct ParticleInput{
	Particle particles[312];
	uint4 toRemove[312/4];
	float3 cameraPos;
	uint numParticles;
	uint numToRemove;
	uint numPrevParticles;
	uint maxOutputVertices;
	float frameTime;
	float particleSize;
};

cbuffer CSInputBuffer : register(b0) {
	ParticleInput inputBuffer;
}

struct Vertex { // Size of this type is hardcoded in ShaderPipeline.cpp - change this if struct changes
	float3 position;
	float2 texCoords;
	float3 normal;
	float3 tangent;
	float3 bitangent;
};

RWStructuredBuffer<Vertex> CSOutputBuffer: register(u10);

struct ParticlePhysics {
	float3 velocity;
	float3 acceleration;
	float3 position;
	float size;
};

RWStructuredBuffer<ParticlePhysics> CSPhysicsBuffer: register(u11) : SAIL_IGNORE;

void createParticle(float3 v0, float3 v1, float3 v2, float3 v3, int particleIndex) {
	int v0Index = particleIndex * 6;
	
	// Triangle 1
	CSOutputBuffer[v0Index].position = v0;
	CSOutputBuffer[v0Index].texCoords = float2(0.f, 0.f);
	CSOutputBuffer[v0Index].normal = 0.f;
	CSOutputBuffer[v0Index].tangent = 0.f;
	CSOutputBuffer[v0Index].bitangent = 0.f;
	
	CSOutputBuffer[v0Index + 1].position = v1;
	CSOutputBuffer[v0Index + 1].texCoords = float2(0.f, 1.f);
	CSOutputBuffer[v0Index + 1].normal = 0.f;
	CSOutputBuffer[v0Index + 1].tangent = 0.f;
	CSOutputBuffer[v0Index + 1].bitangent = 0.f;
	
	CSOutputBuffer[v0Index + 2].position = v2;
	CSOutputBuffer[v0Index + 2].texCoords = float2(1.f, 0.f);
	CSOutputBuffer[v0Index + 2].normal = 0.f;
	CSOutputBuffer[v0Index + 2].tangent = 0.f;
	CSOutputBuffer[v0Index + 2].bitangent = 0.f;
	
	// Triangle 2
	CSOutputBuffer[v0Index + 3].position = v2;
	CSOutputBuffer[v0Index + 3].texCoords = float2(1.f, 0.f);
	CSOutputBuffer[v0Index + 3].normal = 0.f;
	CSOutputBuffer[v0Index + 3].tangent = 0.f;
	CSOutputBuffer[v0Index + 3].bitangent = 0.f;
	
	CSOutputBuffer[v0Index + 4].position = v1;
	CSOutputBuffer[v0Index + 4].texCoords = float2(0.f, 1.f);
	CSOutputBuffer[v0Index + 4].normal = 0.f;
	CSOutputBuffer[v0Index + 4].tangent = 0.f;
	CSOutputBuffer[v0Index + 4].bitangent = 0.f;
	
	CSOutputBuffer[v0Index + 5].position = v3;
	CSOutputBuffer[v0Index + 5].texCoords = float2(1.f, 1.f);
	CSOutputBuffer[v0Index + 5].normal = 0.f;
	CSOutputBuffer[v0Index + 5].tangent = 0.f;
	CSOutputBuffer[v0Index + 5].bitangent = 0.f;
}

void updateVertices(int particleIndex) {
	int v0Index = particleIndex * 6;
	
	float3 particleCam = CSPhysicsBuffer[particleIndex].position - inputBuffer.cameraPos;
	float3 billboardRight = normalize(cross(particleCam, float3(0.f, 1.0f, 0.f)));
	float3 billboardUp = normalize(cross(billboardRight, particleCam));
	
	CSOutputBuffer[v0Index].position = CSPhysicsBuffer[particleIndex].position - CSPhysicsBuffer[particleIndex].size * billboardRight + CSPhysicsBuffer[particleIndex].size * billboardUp;
	CSOutputBuffer[v0Index + 1].position = CSPhysicsBuffer[particleIndex].position - CSPhysicsBuffer[particleIndex].size * billboardRight - CSPhysicsBuffer[particleIndex].size * billboardUp;
	CSOutputBuffer[v0Index + 2].position = CSPhysicsBuffer[particleIndex].position + CSPhysicsBuffer[particleIndex].size * billboardRight + CSPhysicsBuffer[particleIndex].size * billboardUp;
	CSOutputBuffer[v0Index + 3].position = CSPhysicsBuffer[particleIndex].position + CSPhysicsBuffer[particleIndex].size * billboardRight + CSPhysicsBuffer[particleIndex].size * billboardUp;
	CSOutputBuffer[v0Index + 4].position = CSPhysicsBuffer[particleIndex].position - CSPhysicsBuffer[particleIndex].size * billboardRight - CSPhysicsBuffer[particleIndex].size * billboardUp;
	CSOutputBuffer[v0Index + 5].position = CSPhysicsBuffer[particleIndex].position + CSPhysicsBuffer[particleIndex].size * billboardRight - CSPhysicsBuffer[particleIndex].size * billboardUp;
	
	// for (uint i = 0; i < 6; i++) {
	// 	CSOutputBuffer[v0Index + i].normal = normalize(-particleCam);
	// }
	
}

void updatePhysics(int particleIndex, float dt) {
	float3 oldVelocity = CSPhysicsBuffer[particleIndex].velocity;
	CSPhysicsBuffer[particleIndex].velocity += CSPhysicsBuffer[particleIndex].acceleration * dt;
	CSPhysicsBuffer[particleIndex].position += (oldVelocity + CSPhysicsBuffer[particleIndex].velocity) * 0.5 * dt;
	
	updateVertices(particleIndex);
}

void removeParticle(uint particleToRemoveIndex) {
	uint bigIndex = floor(particleToRemoveIndex / 4);
	uint smallIndex = particleToRemoveIndex % 4;
	uint swapIndex = 0;
	if (inputBuffer.toRemove[bigIndex][smallIndex] < inputBuffer.numPrevParticles - inputBuffer.numToRemove) {
		swapIndex = inputBuffer.numPrevParticles - particleToRemoveIndex - 1;
		uint counter = 0;
		while (inputBuffer.toRemove[floor((inputBuffer.numToRemove - 1 - counter)/4)][(inputBuffer.numToRemove - 1 - counter)%4] >= swapIndex && counter < inputBuffer.numToRemove - 1) {
			swapIndex--;
			counter++;
		}
		
		if (swapIndex > inputBuffer.toRemove[bigIndex][smallIndex]) {
			CSPhysicsBuffer[inputBuffer.toRemove[bigIndex][smallIndex]] = CSPhysicsBuffer[swapIndex];
			for (uint j = 0 ; j < 6; j++) {
				CSOutputBuffer[inputBuffer.toRemove[bigIndex][smallIndex] * 6 + j] = CSOutputBuffer[swapIndex * 6 + j];
			}
		} else {
			swapIndex = inputBuffer.toRemove[bigIndex][smallIndex];
		}
	} else {
		swapIndex = inputBuffer.toRemove[bigIndex][smallIndex];
	}
	
	for (uint j = 0 ; j < 6; j++) {
		CSPhysicsBuffer[swapIndex].velocity = 0.f;
		CSPhysicsBuffer[swapIndex].acceleration = 0.f;
		CSOutputBuffer[swapIndex * 6 + j].position = -999999.0f;
	}
	
}

#define X_THREADS 8
#define Y_THREADS 8
#define Z_THREADS 1

struct ComputeShaderInput
{
    uint3 GroupID           : SV_GroupID;           // 3D index of the thread group in the dispatch.
    uint3 GroupThreadID     : SV_GroupThreadID;     // 3D index of local thread ID in a thread group.
    uint3 DispatchThreadID  : SV_DispatchThreadID;  // 3D index of global thread ID in the dispatch.
    uint  GroupIndex        : SV_GroupIndex;        // Flattened local index of the thread within a thread group.
};

[numthreads(X_THREADS, Y_THREADS, Z_THREADS)]
void CSMain(ComputeShaderInput IN) {
	// Change these if multiple thread groups are added
	uint totalThreads = X_THREADS * Y_THREADS * Z_THREADS;
	uint thisThread = IN.GroupIndex;
	
	uint stride = totalThreads;
	
	for (uint i = thisThread; i < inputBuffer.numToRemove; i += stride) {
		removeParticle(i);
	}
	
	AllMemoryBarrierWithGroupSync();
	
	//Find how many particles should be spawned
	uint particleBufferSizeLeft = floor(inputBuffer.maxOutputVertices / 6) - (inputBuffer.numPrevParticles - inputBuffer.numToRemove);
	uint particlesToSpawn = inputBuffer.numParticles;
	if (inputBuffer.numParticles > particleBufferSizeLeft) {
		particlesToSpawn = particleBufferSizeLeft;
	}
	
    for (i = thisThread; i < particlesToSpawn; i += stride) {
		float3 v0, v1, v2, v3;
		
		v0 = inputBuffer.particles[i].position + float3(-0.1, 0.1, 0.0);
		v1 = inputBuffer.particles[i].position + float3(-0.1, -0.1, 0.0);
		v2 = inputBuffer.particles[i].position + float3(0.1, 0.1, 0.0);
		v3 = inputBuffer.particles[i].position + float3(0.1, -0.1, 0.0);
		
		CSPhysicsBuffer[inputBuffer.numPrevParticles - inputBuffer.numToRemove + i].acceleration = inputBuffer.particles[i].acceleration;
		CSPhysicsBuffer[inputBuffer.numPrevParticles - inputBuffer.numToRemove + i].velocity = inputBuffer.particles[i].velocity;
		CSPhysicsBuffer[inputBuffer.numPrevParticles - inputBuffer.numToRemove + i].position = inputBuffer.particles[i].position;
		CSPhysicsBuffer[inputBuffer.numPrevParticles - inputBuffer.numToRemove + i].size = inputBuffer.particleSize;
		
		createParticle(v0, v1, v2, v3, inputBuffer.numPrevParticles - inputBuffer.numToRemove + i);
		
		// Physics for new particle (accounts for how far into the frame the particle was created)
		updatePhysics(inputBuffer.numPrevParticles - inputBuffer.numToRemove + i, inputBuffer.particles[i].spawnTime);
	}
	
	// Physics for all prevous particles
	for (i = thisThread; i < inputBuffer.numPrevParticles - inputBuffer.numToRemove; i += stride) {
		updatePhysics(i, inputBuffer.frameTime);
	}
}
