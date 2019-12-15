#define HLSL
#include "Common_hlsl_cpp.hlsl"

RaytracingAccelerationStructure gRtScene 			: register(t0);
Texture2D<float4> sys_brdfLUT 						: register(t5);

// Gbuffer inputs/outputs
RWTexture2D<float4> gbuffer_normals 				: register(u0);
RWTexture2D<float4> gbuffer_albedo 				    : register(u1);
RWTexture2D<float4> gbuffer_metalnessRoughnessAO 	: register(u2);
Texture2D<float2>   gbuffer_motionVectors			: register(t10);
Texture2D<float>    gbuffer_depth 					: register(t11);

RWTexture2D<float4> lOutputAlbedo		 		: register(u3);		// RGB
RWTexture2D<float4> lOutputNormals 				: register(u4); 	// XYZ
RWTexture2D<float4> lOutputMetalnessRoughnessAO : register(u5); 	// Metalness/Roughness/AO
RWTexture2DArray<float2> lOutputShadows 		: register(u6); 	// Shadows first bounce/shadows second bounce for all lights
RWTexture2D<float4> lOutputPositionsOne			: register(u7);		// XYZ - world space positions for first bounce
RWTexture2D<float4> lOutputPositionsTwo			: register(u8);		// XYZ - world space positions for second bounce
RWTexture2D<float4> lOutputBloom 				: register(u9); 	// RGB - Writes all colors that should later be bloomed
Texture2DArray<float2> InputShadowsLastFrame 	: register(t20); 	// last frame Shadows first bounce/last frame shadows second bounce

ConstantBuffer<SceneCBuffer> CB_SceneData : register(b0, space0);
ConstantBuffer<MeshCBuffer> CB_MeshData : register(b1, space0);

StructuredBuffer<Vertex> vertices : register(t1, space0);
StructuredBuffer<uint> indices : register(t1, space1);
StructuredBuffer<float3> metaballs : register(t1, space2);

StructuredBuffer<uint> waterData : register(t6, space0);

// Closest hit textures
Texture2D<float4> sys_texAlbedo : register(t2);
Texture2D<float4> sys_texNormal : register(t3);
Texture2D<float4> sys_texMetalnessRoughnessAO : register(t4);

SamplerState ss : register(s0);
SamplerState motionSS : register(s1);

#define RAYTRACING
#define RAYTRACER_HARD_SHADOWS
#include "Utils.hlsl"
#include "WaterOnSurface.hlsl"
#include "shading/PBR.hlsl"

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

float getShadowAmount(inout uint seed, float3 worldPosition, float3 worldNormal, inout int inOutlightIndex) {
	// Shoots a ray towards a random point on the light

	const uint numSamples = 1; // Rays per light source per pixel

	bool skip = false;
	PointlightInput p;
	// Find the next applicable light
	bool foundLight = false;
	while (true) {
		skip = false;
		if (inOutlightIndex >= NUM_TOTAL_LIGHTS) {
			// No more applicable lights found!
			inOutlightIndex = -1;
			return 0.f;
		}
		// First "NUM_POINT_LIGHTS" lights are sampled as point lights and the rest as spot lights
		if (inOutlightIndex < NUM_POINT_LIGHTS) {
			p = CB_SceneData.pointLights[inOutlightIndex];
		} else {
			SpotlightInput spotlight = CB_SceneData.spotLights[inOutlightIndex-NUM_POINT_LIGHTS];
			// Skip pixels outside spotlight cone
			float3 L = normalize(spotlight.position - worldPosition);
			float angle = dot(L, normalize(spotlight.direction));
			if(spotlight.angle == 0 || abs(angle) <= spotlight.angle) {
				skip = true;
			}
			p = (PointlightInput)spotlight;
		}

		// Ignore point light if color is black
		if (all(p.color == 0.0f)) {
			inOutlightIndex++;
			continue;
		}
		// Light found, break!
		break;
	}

	float lightShadowAmount = 0.f;
	if (!skip) {
		float3 toLight = normalize(p.position - worldPosition);
		// Calculate a vector perpendicular to L
		float3 perpL = cross(toLight, float3(0.f, 1.0f, 0.f));
		// Handle case where L = up -> perpL should then be (1,0,0)
		if (all(perpL == 0.0f)) {
			perpL.x = 1.0;
		}
		// Use perpL to get a vector from worldPosition to the edge of the light sphere
		float3 toLightEdge = normalize((p.position+perpL*LIGHT_RADIUS) - worldPosition);
		// Angle between L and toLightEdge. Used as the cone angle when sampling shadow rays
		float coneAngle = acos(dot(toLight, toLightEdge)) * 2.0f;
		float distance = length(p.position - worldPosition);

		[unroll]
		for (int shadowSample = 0; shadowSample < numSamples; shadowSample++) {
			float3 L = Utils::getConeSample(seed, toLight, coneAngle);
			lightShadowAmount += (float)Utils::rayHitAnything(worldPosition, worldNormal, L, distance);
		}
		lightShadowAmount /= numSamples;
	} else {
		// Assume shadow if skipped
		lightShadowAmount = 1.f;
	}

	inOutlightIndex++;

	return lightShadowAmount;
}

[shader("raygeneration")]
void rayGen() {
	uint2 launchIndex = DispatchRaysIndex().xy;

	float2 screenTexCoord = ((float2)launchIndex + 0.5f) / DispatchRaysDimensions().xy;

	// Use G-Buffers to calculate/get world position, normal and texture coordinates for this screen pixel
	// G-Buffers contain data in world space
	float3 worldNormal = gbuffer_normals[launchIndex].rgb * 2.f - 1.f;
	float4 albedoColor = gbuffer_albedo[launchIndex];
	albedoColor = pow(albedoColor, 2.2f); // SRGB

	float4 metalnessRoughnessAO = gbuffer_metalnessRoughnessAO[launchIndex];
	float metalness = metalnessRoughnessAO.r;
	float roughness = metalnessRoughnessAO.g;
	float ao = metalnessRoughnessAO.b;
	float emissivness = pow(1 - metalnessRoughnessAO.a, 2);

	// ---------------------------------------------------
	// --- Calculate world position from depth texture ---
	// ---------------------------------------------------

	// These calculations could be moved to the cpu
	float projectionA = CB_SceneData.farZ / (CB_SceneData.farZ - CB_SceneData.nearZ);
	float projectionB = (-CB_SceneData.farZ * CB_SceneData.nearZ) / (CB_SceneData.farZ - CB_SceneData.nearZ);

	float depth = gbuffer_depth[launchIndex];
	float linearDepth = projectionB / (depth - projectionA);

	float2 screenPos = screenTexCoord * 2.0f - 1.0f;
	screenPos.y = -screenPos.y; // Invert Y for DirectX-style coordinates.

	float3 screenVS = mul(CB_SceneData.clipToView, float4(screenPos, 0.f, 1.0f)).xyz;
	float3 viewRay = float3(screenVS.xy / screenVS.z, 1.f);
	float4 vsPosition = float4(viewRay * linearDepth, 1.0f);

	float3 worldPosition = mul(CB_SceneData.viewToWorld, vsPosition).xyz;
	// ---------------------------------------------------

	// Material
	float3 albedoOne = gbuffer_albedo[launchIndex].rgb;
	float4 mrao = gbuffer_metalnessRoughnessAO[launchIndex];
	float metalnessOne = mrao.r;
	float roughnessOne = mrao.g;
	float aoOne = mrao.b;
	float emissivenessOne = pow(1 - mrao.a, 2);
	float originalAoOne = aoOne;
	// Change material if first bounce color should be water on a surface
	getWaterMaterialOnSurface(albedoOne, metalnessOne, roughnessOne, aoOne, worldNormal, worldPosition);

	RayDesc ray;
	ray.Origin = worldPosition;
	ray.Direction = reflect(worldPosition - CB_SceneData.cameraPosition, worldNormal);
	ray.TMin = 0.00001;
	ray.TMax = 10000.0;

	RayPayload payload = Utils::makeEmptyPayload();
	TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0 /* ray index*/, 0, 0, ray, payload);

	//===========MetaBalls RT START===========
	float3 rayDir;
	float3 origin;
	// Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
	generateCameraRay(launchIndex, origin, rayDir);

	ray.Origin = origin;
	ray.Direction = rayDir;
	// Set TMin to a non-zero small value to avoid aliasing issues due to floating point errors
	// TMin should be kept small to prevent missing geometry at close contact areas
	ray.TMin = 0.00001;
	ray.TMax = 10000.0;

	RayPayload payloadMetaball = Utils::makeEmptyPayload();
	TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, INSTANCE_MASK_METABALLS, 0 /* ray index*/, 0, 0, ray, payloadMetaball);
	//===========MetaBalls RT END===========

	float metaballDepth = dot(normalize(CB_SceneData.cameraDirection), normalize(rayDir) * payloadMetaball.closestTvalue);
	RayPayload finalPayload = payload;
	if (metaballDepth <= linearDepth) { 
		finalPayload = payloadMetaball;
		// Overwrite variables used for shadow rays
		worldPosition = payloadMetaball.worldPositionOne;
		worldNormal = payloadMetaball.normalOne;

		albedoOne = payloadMetaball.albedoOne.rgb;
		metalnessOne = payloadMetaball.metalnessRoughnessAOOne.r;
		roughnessOne = payloadMetaball.metalnessRoughnessAOOne.g;
		aoOne = payloadMetaball.metalnessRoughnessAOOne.b;
		emissivenessOne = 0.f;
	}

	// Second bounce information
	float3 albedoTwo = finalPayload.albedoTwo.rgb;
	float3 worldNormalTwo = finalPayload.normalTwo.rgb;
	mrao = finalPayload.metalnessRoughnessAOTwo;
	float metalnessTwo = mrao.r;
	float roughnessTwo = mrao.g;
	float aoTwo = mrao.b;
	float emissivenessTwo = pow(1 - mrao.a, 2);
	float3 worldPositionTwo = finalPayload.worldPositionTwo;
	// Change material if second bounce color should be water on a surface
	getWaterMaterialOnSurface(albedoTwo, metalnessTwo, roughnessTwo, aoTwo, worldNormalTwo, worldPositionTwo);

	//////////////////////////
	//     Hard shadows     //
	//////////////////////////
	if (CB_SceneData.doHardShadows) {
		// Shade scene

		PBRPixel pixelOne;
		pixelOne.worldPosition = worldPosition;
		pixelOne.worldNormal = worldNormal;
		pixelOne.albedo = albedoOne;
		pixelOne.emissiveness = emissivenessOne;
		pixelOne.metalness = metalnessOne;
		pixelOne.roughness = roughnessOne;
		pixelOne.ao = aoOne;

		PBRPixel pixelTwo;
		pixelTwo.worldPosition = worldPositionTwo;
		pixelTwo.worldNormal = worldNormalTwo;
		pixelTwo.albedo = albedoTwo;
		pixelTwo.emissiveness = emissivenessTwo;
		pixelTwo.metalness = metalnessTwo;
		pixelTwo.roughness = roughnessTwo;
		pixelTwo.ao = aoTwo;

		pixelOne.invViewDir = CB_SceneData.cameraPosition - pixelOne.worldPosition;
    	pixelTwo.invViewDir = pixelOne.worldPosition - pixelTwo.worldPosition;

		PBRScene scene;
		// Shade the reflection
		scene.pointLights = CB_SceneData.pointLights;
		scene.spotLights = CB_SceneData.spotLights;
		scene.brdfLUT = sys_brdfLUT;
		scene.sampler = ss;

		float4 secondBounceColor = pbrShade(scene, pixelTwo, -1.f);
		// Shade the first hit
		float4 outputColor = pbrShade(scene, pixelOne, secondBounceColor.rgb);
		// float4 outputColor = secondBounceColor;
		lOutputPositionsOne[launchIndex] = outputColor;

		// Write bloom pass input
		lOutputBloom[launchIndex] = float4((length(outputColor.rgb) > 1.0f) ? clamp(outputColor.rgb, 0.f, 3.f) : 0.f, 1.0f);

		return; // Stop here, all code below is for soft shadows
	}

#ifdef ALLOW_SOFT_SHADOWS
	//////////////////////////
	//     Soft shadows     //
	//////////////////////////

	// Get shadow from first bounce
	// Initialize a random seed
	uint randSeed = Utils::initRand( DispatchRaysIndex().x + DispatchRaysIndex().y * DispatchRaysDimensions().x, CB_SceneData.frameCount );

	// Temporal filtering via an exponential moving average
	float alpha = 0.3f; // Temporal fade, trading temporal stability for lag
	// Reproject screenTexCoord to where it was last frame
	float2 motionVector = gbuffer_motionVectors.SampleLevel(ss, screenTexCoord, 0);
	motionVector.y = 1.f - motionVector.y;
	motionVector = motionVector * 2.f - 1.0f;

	float totalShadowAmount = 0.f;
	float2 reprojectedTexCoord = screenTexCoord - motionVector;

	uint shadowTextureIndex = 0;
	int lightIndex = 0;
	while (shadowTextureIndex < CB_SceneData.numShadowTextures) {
		float firstBounceShadow = getShadowAmount(randSeed, worldPosition, worldNormal, lightIndex);
		if (lightIndex == -1) {
			// No more lights available!
			break;
		}

		float2 cLast = InputShadowsLastFrame.SampleLevel(motionSS, float3(reprojectedTexCoord, shadowTextureIndex), 0).rg;
		// float2 cLast = 0.0f;
		float2 shadow = float2(firstBounceShadow, finalPayload.shadowTwo[shadowTextureIndex]);
		shadow = alpha * (1.0f - shadow) + (1.0f - alpha) * cLast;
		lOutputShadows[uint3(launchIndex, shadowTextureIndex)] = shadow;
		totalShadowAmount += firstBounceShadow;
		shadowTextureIndex++;
	}
	totalShadowAmount /= max(NUM_SHADOW_TEXTURES, 1);

	// Interpolation to shade water in shadows nicely, currently done in shadepass instead
	// totalShadowAmount = 1 - totalShadowAmount * totalShadowAmount;
	// aoOne = lerp(aoOne, originalAoOne, totalShadowAmount);
	// albedoOne.x = totalShadowAmount;
	// albedoOne.yz = 0.f;

	// Overwrite gbuffers
	gbuffer_albedo[launchIndex] = float4(albedoOne, 1.0f);
	gbuffer_normals[launchIndex] = float4(worldNormal * 0.5f + 0.5f, 1.0f);
	gbuffer_metalnessRoughnessAO[launchIndex] = float4(metalnessOne, roughnessOne, aoOne, emissivenessOne);
	// Write outputs
	lOutputAlbedo[launchIndex] = float4(albedoTwo, 1.0f);
	lOutputNormals[launchIndex] = float4(worldNormalTwo * 0.5f + 0.5f, 1.0f);
	lOutputMetalnessRoughnessAO[launchIndex] = float4(metalnessTwo, roughnessTwo, aoTwo, emissivenessTwo);
	lOutputPositionsOne[launchIndex] = float4(worldPosition, 1.0f);
	lOutputPositionsTwo[launchIndex] = float4(worldPositionTwo, 1.0f);
#endif
}

[shader("miss")]
void miss(inout RayPayload payload) {
	payload.albedoTwo = float4(0.f, 0.f, 0.f, 1.0f);
	payload.closestTvalue = 100000;
}

float4 getAlbedo(MeshData data, float2 texCoords) {
	float4 color = float4(data.color.rgb, 0.f);
	if (data.flags & MESH_HAS_ALBEDO_TEX) {
		float4 sampled = sys_texAlbedo.SampleLevel(ss, texCoords, 0);
		// color.rgb *= pow(sampled.rgb, 2.2f); // SRGB conversion
		color.rgb *= sampled.rgb; // No SRGB conversion
		color.a = sampled.a; // Team color amount is stored in alpha, dont do SRGB conversion
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
	float3 teamColor = CB_SceneData.teamColors[teamColorID].rgb;
	
	// Apply teamcolor depending on alpha. 0 = fully team color
	if (albedoColor.a < 1.0f) {
		float f = 1 - albedoColor.a;
		albedoColor = float4(albedoColor.rgb * (1 - f) + teamColor * f, albedoColor.a);
	}

	float4 metalnessRoughnessAO = getMetalnessRoughnessAO(CB_MeshData.data[blasIndex], texCoords);

	float3 worldPosition = Utils::HitWorldPosition();
	
	// Initialize a random seed
	uint randSeed = Utils::initRand( DispatchRaysIndex().x + DispatchRaysIndex().y * DispatchRaysDimensions().x, CB_SceneData.frameCount );

#ifdef ALLOW_SOFT_SHADOWS	
	if (!CB_SceneData.doHardShadows) {
		// Soft shadows only
		// Write shadows for each light
		int shadowTextureIndex = 0;
		int lightIndex = 0;
		[unroll] // THIS IS REQUIRED FOR SOME REASON
		while (shadowTextureIndex < NUM_SHADOW_TEXTURES) {
			float shadowAmount = getShadowAmount(randSeed, worldPosition, normalInWorldSpace, lightIndex);
			if (lightIndex == -1 || shadowTextureIndex >= CB_SceneData.numShadowTextures) {
				// No more lights available!
				break;
			}
			payload.shadowTwo[shadowTextureIndex] = shadowAmount;
			shadowTextureIndex++;
		}
	}
#endif

	payload.albedoTwo.rgb = albedoColor.rgb; // TODO: store alpha and use as team color amount
	payload.normalTwo = normalInWorldSpace;
	payload.metalnessRoughnessAOTwo = metalnessRoughnessAO;
	payload.worldPositionTwo = worldPosition;
}


[shader("closesthit")]
void closestHitProcedural(inout RayPayload payload, in ProceduralPrimitiveAttributes attribs) {
	payload.recursionDepth++;
	payload.closestTvalue = RayTCurrent();

	float3 normalInWorldSpace = normalize(mul(ObjectToWorld3x4(), float4(attribs.normal.xyz, 0.f)));
	float refractIndex = 1.1f;
	RayPayload reflect_payload = payload;
	RayPayload refract_payload = payload;
	float3 reflectVector = reflect(WorldRayDirection(), attribs.normal.xyz);
	float3 refractVector = refract(WorldRayDirection(), attribs.normal.xyz, refractIndex); //Refract index of water is 1.333, so thats what we will use. nope :)

	RayDesc reflectRaydesc = Utils::getRayDesc(reflectVector);
	RayDesc reftractRaydesc = Utils::getRayDesc(refractVector);
	reflectRaydesc.Origin += reflectRaydesc.Direction * 0.0001;
	reftractRaydesc.Origin += reftractRaydesc.Direction * 0.0001;

	if (payload.recursionDepth == 1) {
		TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, INSTANCE_MASK_DEFAULT & ~INSTANCE_MASK_METABALLS, 0, 0, 0, reflectRaydesc, reflect_payload);
		TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, INSTANCE_MASK_DEFAULT & ~INSTANCE_MASK_METABALLS, 0, 0, 0, reftractRaydesc, refract_payload);

		float4 reflect_color = reflect_payload.albedoTwo;
		reflect_color.b += 0.01f;
		reflect_color =  saturate(reflect_color);

		float4 refract_color = refract_payload.albedoTwo;
		refract_color.b += 0.3f;
		refract_color = saturate(refract_color);

		float3 hitToCam = CB_SceneData.cameraPosition - Utils::HitWorldPosition();
		float refconst = pow(abs(dot(normalize(hitToCam), normalInWorldSpace)), 2);

		// float4 finaldiffusecolor = saturate((refract_color * refconst + reflect_color * (1- refconst)));
		float4 finaldiffusecolor = refract_color * 0.8f + reflect_color * 0.2f;
		finaldiffusecolor.a = 1;

		/////////////////////////
		payload.albedoOne = float4(refract_color.rgb, 1.0f);
		payload.normalOne = normalInWorldSpace;
		payload.metalnessRoughnessAOOne = float4(1.f, 0.f, 1.f, 1.f);
		payload.worldPositionOne = Utils::HitWorldPosition();

		payload.albedoTwo = float4(reflect_color.rgb, 1.0f);
		payload.normalTwo = reflect_payload.normalTwo;
		payload.metalnessRoughnessAOTwo = reflect_payload.metalnessRoughnessAOTwo;
		payload.worldPositionTwo = reflect_payload.worldPositionTwo;

	} else {
		payload.albedoTwo = float4(0.0f, 0.0f, .01f, 1.0f);
		payload.normalTwo = normalInWorldSpace;
		payload.metalnessRoughnessAOTwo = float4(1.f, 1.f, 1.f, 1.f);
		payload.worldPositionTwo = Utils::HitWorldPosition();
	}
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
float CalculateMetaballsPotential(in uint index, in float3 position, in uint start, in uint end) {
	//return 1;
	float sumFieldPotential = 0;

	uint mid = index;
	uint nWeights = 10;

	uint start2 = mid - nWeights;
	uint end2 = mid + nWeights;

	if (start2 < start || start2 > mid)//TODO:: GROUP START
		start2 = start;

	if (end2 > end)
		end2 = end;

	for (int i = start2; i < end2; i++) {
		sumFieldPotential += CalculateMetaballPotential(position, metaballs[i], METABALL_RADIUS);
	}

	return sumFieldPotential;
}

bool isInsideIsoSurface(in uint index, in float3 position, in uint start, in uint end, in float t) {
	float sumFieldPotential = 0;
	uint mid = index;
	uint nWeights = 10;

	uint start2 = mid - nWeights;
	uint end2 = mid + nWeights;

	if (start2 < start || start2 > mid)
		start2 = start;

	if (end2 > end)
		end2 = end;

	for (int i = start2; i < end2; i++) {
		sumFieldPotential += CalculateMetaballPotential(position, metaballs[i], METABALL_RADIUS);
		if (sumFieldPotential > t) {
			return true;
		}
	}

	return false;
}

// Calculate a normal via central differences.
float3 CalculateMetaballsNormal(in uint index, in float3 position, in uint start, in uint end) {
	float e = 0.5773 * 0.00001;
	return normalize(float3(
		CalculateMetaballsPotential(index, position + float3(-e, 0, 0), start, end) -
		CalculateMetaballsPotential(index, position + float3(e, 0, 0), start, end),
		CalculateMetaballsPotential(index, position + float3(0, -e, 0), start, end) -
		CalculateMetaballsPotential(index, position + float3(0, e, 0), start, end),
		CalculateMetaballsPotential(index, position + float3(0, 0, -e), start, end) -
		CalculateMetaballsPotential(index, position + float3(0, 0, e), start, end)));
}

struct Ballhit {
	float tmin;
	float tmax;
	int index;
};

bool Step(in Ballhit hit, in RayDesc rayWorld, in uint start, in uint end){
		
	float tmin = hit.tmin, tmax = hit.tmax;
	unsigned int MAX_LARGE_STEPS = 32;//If these steps dont hit any metaball no hit is reported.
	unsigned int MAX_SMALL_STEPS = 32;//If a large step hit a metaball, use small steps to adjust go backwards

	ProceduralPrimitiveAttributes attr;
	float t = tmin;
	float minTStep = (tmax - tmin) / (MAX_LARGE_STEPS / 1.0f);
	unsigned int iStep = 0;

	float3 currPos = rayWorld.Origin + t * rayWorld.Direction;
	while (iStep++ < MAX_LARGE_STEPS) {
		float sumFieldPotential = CalculateMetaballsPotential(hit.index, currPos, start, end); // Sum of all metaball field potentials.

		const float Threshold = 0.90f;

		if (sumFieldPotential >= Threshold) {
			float restep_step = minTStep / 2;
			int restep_step_dir = -1;

			for (int i = 0; i < MAX_SMALL_STEPS; i++) {
				t += restep_step * restep_step_dir;
				currPos = rayWorld.Origin + t * rayWorld.Direction;
				float sumFieldPotential_recomp = CalculateMetaballsPotential(hit.index, currPos, start, end); // Sum of all metaball field potentials.
				if (sumFieldPotential_recomp >= Threshold) {
					restep_step *= 0.5;
					restep_step_dir = -1;
				} else {
					restep_step *= 0.8;
					restep_step_dir = 1;
				}
			}

			attr.normal = float4(CalculateMetaballsNormal(hit.index, currPos, start, end), 0);
			ReportHit(t, 0, attr);
			return true;
		}

		t += minTStep;
		currPos = rayWorld.Origin + t * rayWorld.Direction;
	}

	return false;
}

float3 ProjectToRay(in float3 p, in RayDesc ray) {
	float3 l = p - ray.Origin;
	return (dot(l, ray.Direction)) * ray.Direction + ray.Origin;
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
	uint groupIndex = InstanceID();
	uint groupStart = CB_SceneData.metaballGroup[groupIndex].start;
	uint nballs = CB_SceneData.metaballGroup[groupIndex].size;
	uint groupEnd = groupStart + nballs;

	////////////////////////////////////////////////////////////////////////////////////////
	ProceduralPrimitiveAttributes attr;
	for (uint i = groupStart; i < groupEnd; i++) {
		if (intersectSphere(rayWorld, metaballs[i], METABALL_RADIUS, min, max, dummy)) {
			
			float3 projection = ProjectToRay(metaballs[i], rayWorld);		
			if (isInsideIsoSurface(i, projection, groupStart, groupEnd, 0.9f)) {
				//Report hit here.
				//This is only close to the metaball and not at the edge of it which will result in both wrong normal and hit position.
				//It might however be good enught for small and fast moving metaballs
				attr.normal = float4(CalculateMetaballsNormal(i, projection, groupStart, groupEnd), 0);
				ReportHit(length(projection - rayWorld.Origin), 0, attr);
			}
		}
	}
	////////////////////////////////////////////////////////////////////////////////////////
}
