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
typedef UINT32 uint;

namespace DXRShaderCommon {

#endif


#define MAX_RAY_RECURSION_DEPTH 15
#define MAX_INSTANCES 400
#define NUM_POINT_LIGHTS 12
#define NUM_TEAM_COLORS 12
#define MAX_NUM_METABALLS 500
#define METABALL_RADIUS 0.12f
#define MAX_DECALS 100

#define WATER_GRID_X 350 / 4  // Should be approximately 5 per world unit
#define WATER_GRID_Y 37
#define WATER_GRID_Z 350
#define WATER_ARR_SIZE (WATER_GRID_X * WATER_GRID_Y * WATER_GRID_Z)

static const uint MESH_NO_FLAGS				 			= 	0;
static const uint MESH_USE_INDICES 						= 	1 << 0;
static const uint MESH_HAS_ALBEDO_TEX 					= 	1 << 1;
static const uint MESH_HAS_NORMAL_TEX 					= 	1 << 2;
static const uint MESH_HAS_METALNESS_ROUGHNESS_AO_TEX	= 	1 << 3;

#define INSTACE_MASK_DEFAULT 0xF0
#define INSTACE_MASK_METABALLS 0x01
#define INSTACE_MASK_CAST_SHADOWS 0x02

struct RayPayload {
	float4 color;
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

struct PointLightInput {
	float3 color;
	float padding;
	float3 position;
    float attConstant;
    float attLinear;
    float attQuadratic;
	float2 padding2;
};

struct SpotlightInput {
	float3 color;
	float attConstant;
	float3 position;
	float attLinear;
	float3 direction;
	float attQuadratic;

	float angle;
	float padding1, padding2, padding3;
};

// Properties set once for the scene
struct SceneCBuffer {
	float4x4 projectionToWorld;
	float4x4 viewToWorld;
	float4x4 clipToView;
	float3 cameraPosition;
	bool doTonemapping;
	float3 cameraDirection;
	uint nMetaballs;
    uint nDecals;
	float nearZ;
	float farZ;
	float padding2;
    PointLightInput pointLights[NUM_POINT_LIGHTS];
    SpotlightInput spotLights[NUM_POINT_LIGHTS];
	float4 teamColors[NUM_TEAM_COLORS];

	// Water voxel data
	float3 mapSize;
	float padding3;
	float3 mapStart;
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

struct DecalData {
    float3 position;
	float padding0;
    float3 halfSize;
	float padding1;
    float4x4 rot;
};
struct DecalCBuffer {
    DecalData data[MAX_DECALS];
};

struct ProceduralPrimitiveAttributes {
	float4 normal;
};

#ifndef HLSL
// C++ only

} // End namespace

#endif

#endif // __COMMON_HLSL__