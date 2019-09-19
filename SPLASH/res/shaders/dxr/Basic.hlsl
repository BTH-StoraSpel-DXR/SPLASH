#include "Utils.hlsl"
#define HLSL
#include "Common_hlsl_cpp.hlsl"

RaytracingAccelerationStructure gRtScene : register(t0);
RWTexture2D<float4> lOutput : register(u0);

ConstantBuffer<SceneCBuffer> CB_SceneData : register(b0, space0);
ConstantBuffer<MeshCBuffer> CB_MeshData : register(b1, space0);
StructuredBuffer<Vertex> vertices : register(t1, space0);
StructuredBuffer<uint> indices : register(t1, space1);

// Texture2DArray<float4> textures : register(t2, space0);
Texture2D<float4> sys_texDiffuse : register(t2);
Texture2D<float4> sys_texNormal : register(t3);
Texture2D<float4> sys_texSpecular : register(t4);

SamplerState ss : register(s0);

// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
inline void generateCameraRay(uint2 index, out float3 origin, out float3 direction) {
	float2 xy = index + 0.5f; // center in the middle of the pixel.
	float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

	// Invert Y for DirectX-style coordinates.
	screenPos.y = -screenPos.y;

	// Unproject the pixel coordinate into a ray.
	float4 world = mul(CB_SceneData.projectionToWorld, float4(screenPos, 0, 1));

	world.xyz /= world.w;
	origin = CB_SceneData.cameraPosition.xyz;
	direction = normalize(world.xyz - origin);
}

[shader("raygeneration")]
void rayGen() {
	float3 rayDir;
	float3 origin;

	uint2 launchIndex = DispatchRaysIndex().xy;
	// Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
	generateCameraRay(launchIndex, origin, rayDir);

	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = rayDir;

	// Set TMin to a non-zero small value to avoid aliasing issues due to floating point errors
	// TMin should be kept small to prevent missing geometry at close contact areas
	ray.TMin = 0.00001;
	ray.TMax = 10000.0;

	RayPayload payload;
	payload.recursionDepth = 0;
	payload.hit = 0;
	payload.color = float4(0,0,0,0);
	TraceRay(gRtScene, 0, 0xFF, 0 /* ray index*/, 0, 0, ray, payload);
	lOutput[launchIndex] = payload.color;

	// lOutput[launchIndex] = float4(1.0f, 0.2f, 0.2f, 1.0f);
}

[shader("miss")]
void miss(inout RayPayload payload) {
	payload.color = float4(0.2f, 0.2f, 0.2f, 1.0f);
}

float4 getColor(MeshData data, float2 texCoords) {
	float4 color = data.color;
	if (data.flags & MESH_HAS_DIFFUSE_TEX)
		color *= sys_texDiffuse.SampleLevel(ss, texCoords, 0);
	if (data.flags & MESH_HAS_NORMAL_TEX)
		color += sys_texNormal.SampleLevel(ss, texCoords, 0) * 0.1f;
	if (data.flags & MESH_HAS_SPECULAR_TEX)
		color += sys_texSpecular.SampleLevel(ss, texCoords, 0) * 0.1f;
	return color;
}

[shader("closesthit")]
void closestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs) {
	payload.recursionDepth++;

	float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
	uint instanceID = InstanceID();
	uint primitiveID = PrimitiveIndex();

	uint verticesPerPrimitive = 3;
	uint i1 = primitiveID * verticesPerPrimitive;
	uint i2 = primitiveID * verticesPerPrimitive + 1;
	uint i3 = primitiveID * verticesPerPrimitive + 2;
	// Use indices if available
	if (CB_MeshData.data[instanceID].flags & MESH_USE_INDICES) {
		i1 = indices[i1];
		i2 = indices[i2];
		i3 = indices[i3];
	}
	Vertex vertex1 = vertices[i1];
	Vertex vertex2 = vertices[i2];
	Vertex vertex3 = vertices[i3];

	float3 normalInLocalSpace = Utils::barrypolation(barycentrics, vertex1.normal, vertex2.normal, vertex3.normal);
	float3 normalInWorldSpace = normalize(mul(ObjectToWorld3x4(), normalInLocalSpace));
	float2 texCoords = Utils::barrypolation(barycentrics, vertex1.texCoords, vertex2.texCoords, vertex3.texCoords);

	// float3 normalInWorldSpace = float3(0,-1,0);

	if (payload.recursionDepth < 2) {
		float3 reflectedDir = reflect(WorldRayDirection(), normalInWorldSpace);
		TraceRay(gRtScene, 0, 0xFF, 0, 0, 0, Utils::getRayDesc(reflectedDir), payload);
		payload.color = payload.color * 0.2f + getColor(CB_MeshData.data[instanceID], texCoords) * 0.8f;

	} else {
		// Max recursion, return color
		payload.color = getColor(CB_MeshData.data[instanceID], texCoords);
	}
}
