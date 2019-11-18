#define HLSL
#include "Common_hlsl_cpp.hlsl"

RaytracingAccelerationStructure gRtScene 			: register(t0);
Texture2D<float4> sys_brdfLUT 						: register(t5);
Texture2D<float4> sys_inTex_normals 				: register(t10);
Texture2D<float4> sys_inTex_albedo 					: register(t11);
Texture2D<float4> sys_inTex_texMetalnessRoughnessAO : register(t12);
Texture2D<float>  sys_inTex_depth 					: register(t13);
// Decal textures
Texture2D<float4> decal_texAlbedo 					: register(t14);
Texture2D<float4> decal_texNormal 					: register(t15);
Texture2D<float4> decal_texMetalnessRoughnessAO 	: register(t16);

RWTexture2D<float4> lOutput 	 : register(u0);
RWTexture2D<float4> lOutputBloom : register(u1);

ConstantBuffer<SceneCBuffer> CB_SceneData : register(b0, space0);
ConstantBuffer<MeshCBuffer> CB_MeshData : register(b1, space0);
ConstantBuffer<DecalCBuffer> CB_DecalData : register(b2, space0);

StructuredBuffer<Vertex> vertices : register(t1, space0);
StructuredBuffer<uint> indices : register(t1, space1);
StructuredBuffer<float3> metaballs : register(t1, space2);
StructuredBuffer<uint> waterData : register(t6, space0);

// Texture2DArray<float4> textures : register(t2, space0);
Texture2D<float4> sys_texAlbedo : register(t2);
Texture2D<float4> sys_texNormal : register(t3);
Texture2D<float4> sys_texMetalnessRoughnessAO : register(t4);

SamplerState ss : register(s0);

#include "Utils.hlsl"
#include "Shading.hlsl"
#include "Decals.hlsl"


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
	uint2 launchIndex = DispatchRaysIndex().xy;

#define TRACE_FROM_GBUFFERS
#ifdef TRACE_FROM_GBUFFERS
	float2 screenTexCoord = ((float2)launchIndex + 0.5f) / DispatchRaysDimensions().xy;

	// Use G-Buffers to calculate/get world position, normal and texture coordinates for this screen pixel
	// G-Buffers contain data in world space
	float3 worldNormal = sys_inTex_normals.SampleLevel(ss, screenTexCoord, 0).rgb * 2.f - 1.f;
	float4 albedoColor = sys_inTex_albedo.SampleLevel(ss, screenTexCoord, 0).rgba;
	
	albedoColor = pow(albedoColor, 2.2f);

	float4 metalnessRoughnessAO = sys_inTex_texMetalnessRoughnessAO.SampleLevel(ss, screenTexCoord, 0);
	float metalness = metalnessRoughnessAO.r;
	float roughness = metalnessRoughnessAO.g;
	float ao = metalnessRoughnessAO.b;
	float emissivness = pow(1 - metalnessRoughnessAO.a, 2);

	// ---------------------------------------------------
	// --- Calculate world position from depth texture ---

	// TODO: move calculations to cpu
	float projectionA = CB_SceneData.farZ / (CB_SceneData.farZ - CB_SceneData.nearZ);
	float projectionB = (-CB_SceneData.farZ * CB_SceneData.nearZ) / (CB_SceneData.farZ - CB_SceneData.nearZ);

	float depth = sys_inTex_depth.SampleLevel(ss, screenTexCoord, 0);
	float linearDepth = projectionB / (depth - projectionA);

	float2 screenPos = screenTexCoord * 2.0f - 1.0f;
	screenPos.y = -screenPos.y; // Invert Y for DirectX-style coordinates.

	float3 screenVS = mul(CB_SceneData.clipToView, float4(screenPos, 0.f, 1.0f)).xyz;
	float3 viewRay = float3(screenVS.xy / screenVS.z, 1.f);

	// float3 viewRay = normalize(float3(screenPos, 1.0f));
	float4 vsPosition = float4(viewRay * linearDepth, 1.0f);

	float3 worldPosition = mul(CB_SceneData.viewToWorld, vsPosition).xyz;
	// ---------------------------------------------------

	RayPayload payload;
	payload.recursionDepth = 1;
	payload.closestTvalue = 0;
	payload.color = float4(0,0,0,0);
	if (worldNormal.x == -1 && worldNormal.y == -1) {
		// Bounding boxes dont need shading
		lOutput[launchIndex] = float4(albedoColor.rgb, 1.0f);
		return;
	} else {
		shade(worldPosition, worldNormal, albedoColor.rgb, emissivness, metalness, roughness, ao, payload);
	}


	//===========MetaBalls RT START===========
	float3 rayDir;
	float3 origin;
	// Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
	generateCameraRay(launchIndex, origin, rayDir);

	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = rayDir;
	// Set TMin to a non-zero small value to avoid aliasing issues due to floating point errors
	// TMin should be kept small to prevent missing geometry at close contact areas
	ray.TMin = 0.00001;
	ray.TMax = 10000.0;

	RayPayload payload_metaball;
	payload_metaball.recursionDepth = 0;
	payload_metaball.closestTvalue = 0;
	payload_metaball.color = float4(0, 0, 0, 0);

	TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, INSTACE_MASK_METABALLS, 0 /* ray index*/, 0, 0, ray, payload_metaball);
	//===========MetaBalls RT END===========

	float metaballDepth = dot(normalize(CB_SceneData.cameraDirection), normalize(rayDir) * payload_metaball.closestTvalue);

	float3 outputColor;
	if (metaballDepth <= linearDepth) {
		outputColor = payload_metaball.color.rgb;
	} else {
		outputColor = payload.color.rgb;

		// Decals are untested since tonemapping was moved, uncomment at own risk
		// float4 totDecalColour = 0.0f;
		// for (uint i = 0; i < CB_SceneData.nDecals; i++) {
		// 	totDecalColour += renderDecal(i, vsPosition.xyz, worldPosition, worldNormal, payload.color);		
		// 	if (!all(totDecalColour == 0.0f)) {
		// 		lOutput[launchIndex] = totDecalColour;
		// 		break;
		// 	}
		// }
	}
	// Perform tonemapping if requested
	// Tonemapping is otherwise done in post processing
	outputColor = (CB_SceneData.doTonemapping) ? tonemap(outputColor) : outputColor;
	// Write outputs
	lOutput[launchIndex] = float4(outputColor, 1.0f);
	lOutputBloom[launchIndex] = float4((length(outputColor) > 1.0f) ? outputColor : 0.f, 1.0f);

#else
	// Fully RT

	float3 rayDir;
	float3 origin;
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
	payload.closestTvalue = 0;

	payload.color = float4(0, 0, 0, 0);
	TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, INSTACE_MASK_DEFAULT & ~INSTACE_MASK_METABALLS, 0 /* ray index*/, 0, 0, ray, payload);

	lOutput[launchIndex] = payload.color;
#endif
}

[shader("miss")]
void miss(inout RayPayload payload) {
	payload.color = float4(0.01f, 0.01f, 0.01f, 1.0f);
	payload.closestTvalue = 1000;
}

float4 getAlbedo(MeshData data, float2 texCoords) {
	float4 color = float4(data.color.rgb, 1.0f);
	if (data.flags & MESH_HAS_ALBEDO_TEX) {		
		color *= sys_texAlbedo.SampleLevel(ss, texCoords, 0);
	}
	
	return color;
}
float4 getMetalnessRoughnessAO(MeshData data, float2 texCoords) {
	float4 color = float4(data.metalnessRoughnessAoScales, 1);
	if (data.flags & MESH_HAS_METALNESS_ROUGHNESS_AO_TEX)
		color *= sys_texMetalnessRoughnessAO.SampleLevel(ss, texCoords, 0);
	return color;
}

[shader("closesthit")]
void closestHitTriangle(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs) {
	payload.recursionDepth++;
	payload.closestTvalue = RayTCurrent();

	float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
	uint blasIndex = InstanceID() & ~(~0U << 10); //Extract vertexbufferID
	uint teamColorID = (InstanceID() >> 10);//Extract teamColorID

	uint primitiveID = PrimitiveIndex();
	
	uint verticesPerPrimitive = 3;
	uint i1 = primitiveID * verticesPerPrimitive;
	uint i2 = primitiveID * verticesPerPrimitive + 1;
	uint i3 = primitiveID * verticesPerPrimitive + 2;
	// Use indices if available
	if (CB_MeshData.data[blasIndex].flags & MESH_USE_INDICES) {
		i1 = indices[i1];
		i2 = indices[i2];
		i3 = indices[i3];
	}
	Vertex vertex1 = vertices[i1];
	Vertex vertex2 = vertices[i2];
	Vertex vertex3 = vertices[i3];

	float2 texCoords = Utils::barrypolation(barycentrics, vertex1.texCoords, vertex2.texCoords, vertex3.texCoords);
	float3 normalInLocalSpace = Utils::barrypolation(barycentrics, vertex1.normal, vertex2.normal, vertex3.normal);
	float3 normalInWorldSpace = normalize(mul(ObjectToWorld3x4(), float4(normalInLocalSpace, 0.f)));

	float3 tangentInLocalSpace = Utils::barrypolation(barycentrics, vertex1.tangent, vertex2.tangent, vertex3.tangent);
	float3 tangentInWorldSpace = normalize(mul(ObjectToWorld3x4(), float4(tangentInLocalSpace, 0.f)));

	float3 bitangentInLocalSpace = Utils::barrypolation(barycentrics, vertex1.bitangent, vertex2.bitangent, vertex3.bitangent);
	float3 bitangentInWorldSpace = normalize(mul(ObjectToWorld3x4(), float4(bitangentInLocalSpace, 0.f)));

	// Create TBN matrix to go from tangent space to world space
	float3x3 tbn = float3x3(
	  tangentInWorldSpace,
	  bitangentInWorldSpace,
	  normalInWorldSpace
	);
	if (CB_MeshData.data[blasIndex].flags & MESH_HAS_NORMAL_TEX) {
		float3 normalSample = sys_texNormal.SampleLevel(ss, texCoords, 0).rgb;
        normalSample.y = 1.0f - normalSample.y;
        normalInWorldSpace = mul(normalize(normalSample * 2.f - 1.f), tbn);
	}

	float4 albedoColor = getAlbedo(CB_MeshData.data[blasIndex], texCoords);
	float a = albedoColor.a;
	float3 teamColor = CB_SceneData.teamColors[teamColorID].rgb;
	
	if (a < 1.0f) {
		float f = 1 - a;
		albedoColor = float4(albedoColor.rgb * (1 - f) + teamColor * f, a);
	}
	albedoColor = float4(pow(albedoColor.rgb, 2.2), a);

	float4 metalnessRoughnessAO = getMetalnessRoughnessAO(CB_MeshData.data[blasIndex], texCoords);
	float metalness = metalnessRoughnessAO.r;
	float roughness = metalnessRoughnessAO.g;
	float ao = metalnessRoughnessAO.b;
	float emissivness = pow(1 - metalnessRoughnessAO.a, 2);

	shade(Utils::HitWorldPosition(), normalInWorldSpace, albedoColor.rgb, emissivness, metalness, roughness, ao, payload, true);
}


[shader("closesthit")]
void closestHitProcedural(inout RayPayload payload, in ProceduralPrimitiveAttributes attribs) {
	payload.recursionDepth++;
	payload.closestTvalue = RayTCurrent();

	float3 normalInWorldSpace = normalize(mul(ObjectToWorld3x4(), float4(attribs.normal.xyz, 0.f)));
	float refractIndex = 1.333f;
	RayPayload reflect_payload = payload;
	RayPayload refract_payload = payload;
	float3 reflectVector = reflect(WorldRayDirection(), attribs.normal.xyz);
	float3 refractVector = refract(WorldRayDirection(), attribs.normal.xyz, refractIndex); //Refract index of water is 1.333, so thats what we will use.

	RayDesc reflectRaydesc = Utils::getRayDesc(reflectVector);
	RayDesc reftractRaydesc = Utils::getRayDesc(refractVector);
	reflectRaydesc.Origin += reflectRaydesc.Direction * 0.0001;
	reftractRaydesc.Origin += reftractRaydesc.Direction * 0.0001;

	if (payload.recursionDepth == 1) {
		TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, INSTACE_MASK_DEFAULT & ~INSTACE_MASK_METABALLS, 0, 0, 0, reflectRaydesc, reflect_payload);
		TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, INSTACE_MASK_DEFAULT & ~INSTACE_MASK_METABALLS, 0, 0, 0, reftractRaydesc, refract_payload);

	} else {
		reflect_payload.color = float4(0.0f, 0.0f, 0.1f,1.0f);
		refract_payload.color = float4(0.0f, 0.0f, 0.f,1.0f);
	}

	float4 reflect_color = reflect_payload.color;
	reflect_color.b += 0.05f;
	reflect_color =  saturate(reflect_color);

	float4 refract_color = refract_payload.color;
	refract_color.b += 0.05f;
	refract_color = saturate(refract_color);

	float3 hitToCam = CB_SceneData.cameraPosition - Utils::HitWorldPosition();
	float refconst = pow(abs(dot(normalize(hitToCam), normalInWorldSpace)), 2);

	float4 finaldiffusecolor = saturate((refract_color * refconst + reflect_color * (1- refconst)));
	finaldiffusecolor.a = 1;
	
	/////////////////////////
	float3 albedoColor = finaldiffusecolor.xyz;
	float metalness = 1;
	float roughness = 1;
	float ao = 1;

	payload.color = phongShade(Utils::HitWorldPosition(), normalInWorldSpace, finaldiffusecolor.xyz);

	return;
}

bool solveQuadratic(in float a, in float b, in float c, inout float x0, inout float x1) {
	float discr = b * b - 4 * a * c;
	if (discr < 0) {
		return false;
	} else if (discr == 0) {
		x0 = x1 = -0.5 * b / a;
	} else {
		float q = (b > 0) ?
			-0.5 * (b + sqrt(discr)) :
			-0.5 * (b - sqrt(discr));
		x0 = q / a;
		x1 = c / q;
	}

	if (x0 > x1) {
		float temp = x0;
		x0 = x1;
		x1 = temp;
	}

	return true;
}

bool intersectSphere(in RayDesc ray, in float3 center, in float radius, out float tmin, out float tmax, out float4 normal) {
	float t0, t1; // solutions for t if the ray intersects 

	// analytic solution
	float3 L = ray.Origin - center;
	float a = dot(ray.Direction, ray.Direction);
	float b = 2 * dot(ray.Direction, L);
	float c = dot(L, L) - radius * radius;
	if (!solveQuadratic(a, b, c, t0, t1)) {
		return false;
	}

	if (t0 > t1) {
		float temp = t0;
		t0 = t1;
		t1 = temp;
	}

	if (t0 < 0) {
		t0 = t1; // if t0 is negative, let's use t1 instead 
		if (t0 < 0) {
			return false; // both t0 and t1 are negative 
		}
	}

	tmin = t0;
	tmax = t1;
	normal = float4(normalize((ray.Origin + tmin * normalize(ray.Direction)) - center), 0);

	return true;
}

// Calculate a magnitude of an influence from a Metaball charge.
// Return metaball potential range: <0,1>
// mbRadius - largest possible area of metaball contribution - AKA its bounding sphere.
float CalculateMetaballPotential(in float3 position, in float3 ballpos, in float ballradius) {
	
	float distance = length(position - ballpos) / ballradius;
	////float3 rel = (ballpos - position) / ballradius;

	if (distance <= 1) {
		/*===Lowest Quality===*/
		//return 1 / distance;

		/*===OK Quality===*/

		float t = (1 - distance * distance);

		return t * t;


		/*===Good Quality===*/
	//	float d = distance;
	//	// Quintic polynomial field function.
	//	// The advantage of this polynomial is having smooth second derivative. Not having a smooth
	//	// second derivative may result in a sharp and visually unpleasant normal vector jump.
	//	// The field function should return 1 at distance 0 from a center, and 1 at radius distance,
	//	// but this one gives f(0) = 0, f(radius) = 1, so we use the distance to radius instead.
	//	d = abs(ballradius) - d;

	//	float r = ballradius;
	//	float q2 = d / r;
	//	float q = q2 * q2 * q2;

	//	float p = 6 * q2 * q2 * q
	//		- 15 * q * q2
	//		+ 10 * q;

	//	return p;
	}
	return 0;
}

// Calculate field potential from all active metaballs.
float CalculateMetaballsPotential(in uint index, in float3 position) {
	//return 1;
	float sumFieldPotential = 0;
	uint nballs = CB_SceneData.nMetaballs;

	int mid = index;
	int nWeights = 30;

	int start = mid - nWeights;
	int end = mid + nWeights;

	if (start < 0)
		start = 0;
	if (end > nballs) 
		end = nballs;

	for (int i = start; i < end - 1; i++) {
		sumFieldPotential += CalculateMetaballPotential(position, metaballs[i], METABALL_RADIUS);
	}

	return sumFieldPotential;
}

// Calculate a normal via central differences.
float3 CalculateMetaballsNormal(in uint index, in float3 position) {
	float e = 0.5773 * 0.00001;
	return normalize(float3(
		CalculateMetaballsPotential(index, position + float3(-e, 0, 0)) -
		CalculateMetaballsPotential(index, position + float3(e, 0, 0)),
		CalculateMetaballsPotential(index, position + float3(0, -e, 0)) -
		CalculateMetaballsPotential(index, position + float3(0, e, 0)),
		CalculateMetaballsPotential(index, position + float3(0, 0, -e)) -
		CalculateMetaballsPotential(index, position + float3(0, 0, e))));
}

struct Ballhit {
	float tmin;
	float tmax;
	int index;
};

bool Step(in Ballhit hit, in RayDesc rayWorld){
		
	float tmin = hit.tmin, tmax = hit.tmax;
	unsigned int MAX_LARGE_STEPS = 32;//If these steps dont hit any metaball no hit is reported.
	unsigned int MAX_SMALL_STEPS = 32;//If a large step hit a metaball, use small steps to adjust go backwards

	ProceduralPrimitiveAttributes attr;
	float t = tmin;
	float minTStep = (tmax - tmin) / (MAX_LARGE_STEPS / 1.0f);
	unsigned int iStep = 0;

	float3 currPos = rayWorld.Origin + t * rayWorld.Direction;
	while (iStep++ < MAX_LARGE_STEPS) {
		float sumFieldPotential = CalculateMetaballsPotential(hit.index, currPos); // Sum of all metaball field potentials.

		const float Threshold = 0.90f;

		if (sumFieldPotential >= Threshold) {
			float restep_step = minTStep / 2;
			int restep_step_dir = -1;

			for (int i = 0; i < MAX_SMALL_STEPS; i++) {
				t += restep_step * restep_step_dir;
				currPos = rayWorld.Origin + t * rayWorld.Direction;
				float sumFieldPotential_recomp = CalculateMetaballsPotential(hit.index, currPos); // Sum of all metaball field potentials.
				if (sumFieldPotential_recomp >= Threshold) {
					restep_step *= 0.5;
					restep_step_dir = -1;
				} else {
					restep_step *= 0.8;
					restep_step_dir = 1;
				}
			}

			attr.normal = float4(CalculateMetaballsNormal(hit.index, currPos), 0);
			ReportHit(t, 0, attr);
			return true;
		}

		t += minTStep;
		currPos = rayWorld.Origin + t * rayWorld.Direction;
	}

	return false;
}

[shader("intersection")]
void IntersectionShader() {
	float startT = 1000;

	RayDesc rayWorld;
	rayWorld.Origin = WorldRayOrigin();
	rayWorld.Direction = WorldRayDirection();
	rayWorld.TMax = 1000000;
	rayWorld.TMin = 0.00001;

	float4 dummy;
	float min;
	float max;
	uint nballs = CB_SceneData.nMetaballs;

	const uint MAX_HITS = 2;
	Ballhit hits[MAX_HITS];
	int nHits = 0;

	for (uint i = 0; i < nballs; i++) {
		if (intersectSphere(rayWorld, metaballs[i], METABALL_RADIUS, min, max, dummy)) {
			hits[nHits].tmin = min;
			hits[nHits].tmax = max + 1;
			hits[nHits].index = i;
			nHits++;
			break;
		}
	}

	for (uint i = 0; i < nballs && nHits < MAX_HITS; i++) {
		uint i2 = nballs - i - 1;
		if (intersectSphere(rayWorld, metaballs[i2], METABALL_RADIUS, min, max, dummy)) {
			hits[nHits].tmin = min;
			hits[nHits].tmax = max + 1;
			hits[nHits].index = i2;
			nHits++;
			break;
		}
	}

	if (nHits == 0)
		return;

	for (int curHit = 0; curHit < nHits; curHit++) {
		if (Step(hits[curHit], rayWorld)) {
			return;
		}
	}
}
