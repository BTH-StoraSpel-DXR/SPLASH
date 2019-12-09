#ifndef __COMMON_HLSL__
#define __COMMON_HLSL__

#ifdef HLSL
// Shader only

#define MERGE(a, b) a##b

#define PI 3.14159265359
#define M_1_PI 0.318309886183790671538

#else
// C++ only

#pragma once

typedef glm::vec2 float2;
typedef glm::vec3 float3;
typedef glm::vec4 float4;
typedef glm::mat3x3 float3x3;
typedef glm::mat4x4 float4x4;
typedef glm::u32vec2 uint2;
typedef glm::u32vec3 uint3;
typedef UINT32 uint;

namespace DXRShaderCommon {

#endif

#define MAX_RAY_RECURSION_DEPTH 15
#define MAX_INSTANCES 400
#define NUM_POINT_LIGHTS 12
#define NUM_SPOT_LIGHTS 12
#define NUM_TOTAL_LIGHTS NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS
#define NUM_TEAM_COLORS 12
#define MAX_NUM_METABALLS 500
#define MAX_NUM_METABALL_GROUPS 16
#define METABALL_RADIUS 0.12f
#define NUM_SHADOW_TEXTURES 14
#define LIGHT_RADIUS 0.08 // TODO: tweak this!

static const uint MESH_NO_FLAGS				 			= 	0;
static const uint MESH_USE_INDICES 						= 	1 << 0;
static const uint MESH_HAS_ALBEDO_TEX 					= 	1 << 1;
static const uint MESH_HAS_NORMAL_TEX 					= 	1 << 2;
static const uint MESH_HAS_METALNESS_ROUGHNESS_AO_TEX	= 	1 << 3;

#define INSTANCE_MASK_DEFAULT 0xF0
#define INSTANCE_MASK_METABALLS 0x01
#define INSTANCE_MASK_CAST_SHADOWS 0x02

struct RayPayload {
	float4 albedoOne;
	float4 albedoTwo;
	float3 normalOne;
	float3 normalTwo;
	float4 metalnessRoughnessAOOne;
	float4 metalnessRoughnessAOTwo;
	float3 worldPositionOne;
	float3 worldPositionTwo;
	float shadowTwo[NUM_SHADOW_TEXTURES];
	uint recursionDepth;
	float closestTvalue;
};
struct ShadowRayPayload {
	bool isHit;
};

struct Vertex {
	float3 position;
	float2 texCoords;
	float3 normal;
	float3 tangent;
	float3 bitangent;
};

struct PointlightInput {
	float3 color;
	float padding1;
	float3 position;
    float reachRadius;
};

struct SpotlightInput {
	// This part must match point light input
	// (all lights are casted to PointLightInput during shading)
	float3 color;
	float padding1;
	float3 position;
    float reachRadius;
	// This part can be unique for each light type
	float3 direction;
	float angle;
};

struct MetaballGroupData {
	uint start;
	uint size;
	float padding1;
	float padding2;
};

// Properties set once for the scene
struct SceneCBuffer {
	float4x4 projectionToWorld;
	float4x4 viewToWorld;
	float4x4 clipToView;
	float3 cameraPosition;
	bool doHardShadows;
	float3 cameraDirection;
	uint nMetaballGroups;
	uint padding;
	float nearZ;
	float farZ;
	float padding2;
    PointlightInput pointLights[NUM_POINT_LIGHTS];
    SpotlightInput spotLights[NUM_SPOT_LIGHTS];
	float4 teamColors[NUM_TEAM_COLORS];
	MetaballGroupData metaballGroup[MAX_NUM_METABALL_GROUPS];

	// Water voxel data
	float3 mapSize;
	float padding3;
	float3 mapStart;
	float padding4;
	uint3 waterArraySize;

	// Random seed stuff
	uint frameCount;
};

// Properties set once per BLAS/Mesh
struct MeshData {
	float4 color;
	float3 metalnessRoughnessAoScales;
	float padding;
	int flags;
	float3 padding2;
};
struct MeshCBuffer {
	MeshData data[MAX_INSTANCES]; // cbuffer min size is 64kb, fill with flags
};

struct ProceduralPrimitiveAttributes {
	float4 normal;
};

#ifndef HLSL
// C++ only

} // End namespace

#endif

#endif // __COMMON_HLSL__