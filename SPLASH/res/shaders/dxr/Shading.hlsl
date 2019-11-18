#include "PBR.hlsl"

#define WATER_ON_WALLS
// #define WATER_DEBUG

float4 phongShade(float3 worldPosition, float3 worldNormal, float3 diffuseColor) {
    float3 shadedColor = float3(0.f, 0.f, 0.f);
	
	float3 ambientCoefficient = float3(0.f, 0.f, 0.f);
	// TODO: read these from model data
	float shininess = 10.0f;
	float specMap = 1.0f;
	float kd = 1.0f;
	float ka = 1.0f;
	float ks = 1.0f;


	//PointLights
	for (uint i = 0; i < NUM_POINT_LIGHTS; i++) {
		PointLightInput p = CB_SceneData.pointLights[i];

		// Ignore point light if color is black
		if (all(p.color == 0.0f)) {
			continue;
		}

		float3 hitToLight = p.position - worldPosition;
		float distanceToLight = length(hitToLight);

		// Dont do any shading if in shadow or light is black
		if (Utils::rayHitAnything(worldPosition, normalize(hitToLight), distanceToLight)) {
			continue;
		}

		float3 hitToCam = CB_SceneData.cameraPosition - worldPosition;

		float diffuseCoefficient = saturate(dot(worldNormal, hitToLight));

		float3 specularCoefficient = float3(0.f, 0.f, 0.f);
		if (diffuseCoefficient > 0.f) {
			float3 r = reflect(-hitToLight, worldNormal);
			r = normalize(r);
			specularCoefficient = pow(saturate(dot(normalize(hitToCam), r)), shininess) * specMap;
		}

		float attenuation = 1.f / (p.attConstant + p.attLinear * distanceToLight + p.attQuadratic * pow(distanceToLight, 2.f));

		shadedColor += (kd * diffuseCoefficient + ks * specularCoefficient) * diffuseColor * p.color * attenuation;
	}

	//Spotlights
	for (uint i = 0; i < NUM_POINT_LIGHTS; i++) {
		SpotlightInput s = CB_SceneData.spotLights[i];

		// Ignore point light if color is black
		if (all(s.color == 0.0f) || s.angle == 0.0f) {
			continue;
		}

		float3 hitToLight = s.position - worldPosition;
		float distanceToLight = length(hitToLight);
		float angle = dot(hitToLight, s.angle);

		if (angle <= s.angle) {
			continue;
		}

		// Dont do any shading if in shadow or light is black
		if (Utils::rayHitAnything(worldPosition, normalize(hitToLight), distanceToLight)) {
			continue;
		}

		float3 hitToCam = CB_SceneData.cameraPosition - worldPosition;

		float diffuseCoefficient = saturate(dot(worldNormal, hitToLight));

		float3 specularCoefficient = float3(0.f, 0.f, 0.f);
		if (diffuseCoefficient > 0.f) {
			float3 r = reflect(-hitToLight, worldNormal);
			r = normalize(r);
			specularCoefficient = pow(saturate(dot(normalize(hitToCam), r)), shininess) * specMap;
		}

		float attenuation = 1.f / (s.attConstant + s.attLinear * distanceToLight + s.attQuadratic * pow(distanceToLight, 2.f));

		shadedColor += (kd * diffuseCoefficient + ks * specularCoefficient) * diffuseColor * s.color * attenuation;
	}


	float3 ambient = diffuseColor * ka * ambientCoefficient;

	return float4(saturate(ambient + shadedColor), 1.0f);
}

void shade(float3 worldPosition, float3 worldNormal, float3 albedo, float emissivness, float metalness, float roughness, float ao, inout RayPayload payload, bool calledFromClosestHit = false, int reflectionBounces = 1, float reflectionAtt = 0.5f) {
	// Ray direction for first ray when cast from GBuffer must be calculated using camera position
	float3 rayDir = (calledFromClosestHit) ? WorldRayDirection() : worldPosition - CB_SceneData.cameraPosition;

	float originalAo = ao;

#ifdef WATER_ON_WALLS
	// =================================================
	//  Render pixel as water if close to a water point
	// =================================================

	static const float3 arrSize = int3(WATER_GRID_X, WATER_GRID_Y, WATER_GRID_Z);
	static const float cutoff = 0.2f;

	float3 cellWorldSize = CB_SceneData.mapSize / arrSize;
	cellWorldSize.x *= 0.25f;

#ifdef WATER_DEBUG
	float3 floatIndMin = ((worldPosition - CB_SceneData.mapStart) / CB_SceneData.mapSize) * arrSize;
	float3 floatIndMax = ((worldPosition - CB_SceneData.mapStart) / CB_SceneData.mapSize) * arrSize;
	int3 ind = floor(( (worldPosition - CB_SceneData.mapStart) / CB_SceneData.mapSize) * arrSize);
	int3 indMin = ind;
	int3 indMax = ind;
#else
	float3 floatIndMin = ((worldPosition - cutoff - CB_SceneData.mapStart) / CB_SceneData.mapSize) * arrSize;
	float3 floatIndMax = ((worldPosition + cutoff - CB_SceneData.mapStart) / CB_SceneData.mapSize) * arrSize;
	int3 indMin = floor(floatIndMin);
	int3 indMax = ceil(floatIndMax);
#endif

	float3 normalOffset = 0.f;
	float sum = 0.f;
	for (int z = indMin.z; z <= indMax.z; z++) {
		for (int y = indMin.y; y <= indMax.y; y++) {
			for (int x = indMin.x; x <= indMax.x; x++) {
				int i = Utils::to1D(int3(x,y,z), arrSize.x, arrSize.y);
				i = clamp(i, 0, floor(WATER_ARR_SIZE) - 1);
				uint packedR = waterData[i];

				uint start = (x == indMin.x) ? floor(((floatIndMin.x - floor(floatIndMin.x)) * 4.f) % 4) : 0;
				uint end = (x == indMax.x) ? floor(((floatIndMax.x - floor(floatIndMax.x)) * 4.f) % 4) : 3;

				[unroll]
				for (uint index = start; index <= end; index++) {
					uint up = Utils::unpackQuarterFloat(packedR, index);
					if (up > 0) {
						half r = up * 0.00392156863h; // That last wierd one is 1 / 255
						// r = 1.0f;
						float3 waterPointWorldPos = (float3(x*4+index,y,z) + 0.5f) * cellWorldSize + CB_SceneData.mapStart;

						float3 dstV = waterPointWorldPos - worldPosition;
						float dstSqrd = dot(dstV, dstV);
						if (dstSqrd <= 0.09f * r) { // cutoff^2, reomve "* r" if CHARGE1 is used below
							// float charge = 1.f-(dstSqrd)*10.f; // CHARGE1: Disregards r
							float charge = 1.f-(dstSqrd * (1.f / r))*10.f; // CHARGE2: can handle changing blob sizes
							sum += charge * charge;
							normalOffset += normalize(-dstV);
							normalOffset += normalize(float3(x*4+index,y,z) + 0.5f - indMin);
						}
					}
				}
			}
		}
	}

	if (sum > 0.8f) {
#ifdef WATER_DEBUG
		payload.color = float4(1.0f, 0.f, 0.f, 1.0f);
		return;
#endif
		float waterOpacity = clamp(sum / 1.f, 0.f, 0.8f);

		// Shade as water

		metalness = lerp(metalness, 1.0f, 			waterOpacity);
		roughness = lerp(roughness, 0.0f, 			waterOpacity);
		ao 		  = lerp(ao, 		0.5f, 			waterOpacity);
		albedo 	  = lerp(albedo, 	albedo * 0.8f,  waterOpacity);

		float height = 1.6f; // normal smoothness / water "height" apperance
		worldNormal += normalize(normalOffset) * clamp( ( -height*sum + 2.f ), 0.0f, 0.8f);
		worldNormal = normalize(worldNormal);
		// metalness = 1.0f;
		// roughness = 0.01f;
		// ao = 0.5f;
	}
#endif
	payload.color = pbrShade(worldPosition, worldNormal, -rayDir, albedo, emissivness, metalness, roughness, ao, originalAo, payload);
	// payload.color = float4(worldNormal * 0.5f + 0.5f, 1.0f);
	// payload.color = phongShade(worldPosition, worldNormal, albedo);
}