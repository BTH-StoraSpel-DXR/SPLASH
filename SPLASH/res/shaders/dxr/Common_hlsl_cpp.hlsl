#ifdef HLSL
// Shader only

#define MERGE(a, b) a##b

#define M_1_PI 0.318309886183790671538

#else
// C++ only

#pragma once

typedef glm::vec2 float2;
typedef glm::vec3 float3;
typedef glm::vec4 float4;
typedef glm::mat4x4 float4x4;
typedef UINT32 uint;

namespace DXRShaderCommon {

#endif


#define MAX_RAY_RECURSION_DEPTH 30
#define MAX_INSTANCES 400

static const uint MESH_NO_FLAGS	 			= 	0;
static const uint MESH_USE_INDICES 			= 	1 << 0;
static const uint MESH_HAS_DIFFUSE_TEX 		= 	1 << 1;
static const uint MESH_HAS_NORMAL_TEX 		= 	1 << 2;
static const uint MESH_HAS_SPECULAR_TEX		= 	1 << 3;

struct RayPayload {
	float4 color;
	uint recursionDepth;
	int hit;
};

struct Vertex {
	float3 position;
	float2 texCoords;
	float3 normal;
	float3 tangent;
	float3 bitangent;
};

// Properties set once for the scene
struct SceneCBuffer {
	float4x4 projectionToWorld;
	float3 cameraPosition;
};

// Properties set once per BLAS/Mesh
struct MeshData {
	float4 color;
	int flags;
	float3 padding;
};
struct MeshCBuffer {
	MeshData data[MAX_INSTANCES]; // cbuffer min size is 64kb, fill with flags
};

#ifndef HLSL
// C++ only

} // End namespace

#endif