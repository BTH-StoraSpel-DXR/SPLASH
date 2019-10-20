#include "PBR.hlsl"

float4 phongShade(float3 worldPosition, float3 worldNormal, float3 diffuseColor) {
    float3 shadedColor = float3(0.f, 0.f, 0.f);
	
	float3 ambientCoefficient = float3(0.f, 0.f, 0.f);
	// TODO: read these from model data
	float shininess = 10.0f;
	float specMap = 1.0f;
	float kd = 1.0f;
	float ka = 1.0f;
	float ks = 1.0f;

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

	float3 ambient = diffuseColor * ka * ambientCoefficient;

	return float4(saturate(ambient + shadedColor), 1.0f);
}

void shade(float3 worldPosition, float3 worldNormal, float3 albedo, float metalness, float roughness, float ao, inout RayPayload payload, bool calledFromClosestHit = false, int reflectionBounces = 1, float reflectionAtt = 0.5f) {
	// Ray direction for first ray when cast from GBuffer must be calculated using camera position
	float3 rayDir = (calledFromClosestHit) ? WorldRayDirection() : worldPosition - CB_SceneData.cameraPosition;

	// =================================================
	//  Render pixel as water if close to a water point
	// =================================================

	static const float3 mapSize = float3(56.f, 10.f, 56.f);
	static const float3 arrSize = float3(WATER_GRID_X, WATER_GRID_Y, WATER_GRID_Z);
	static const float3 mapStart = float3(-3.5f, 0.0f, -3.5f);
	static const float cutoff = 0.3f;

	float3 cellWorldSize = mapSize / arrSize;
	// int3 ind = round(( (worldPosition - mapStart) / mapSize) * arrSize);
	// int3 indMin = ind;
	// int3 indMax = ind;
	int3 indMin = round(( (worldPosition - cutoff - mapStart) / mapSize) * arrSize);
	int3 indMax = round(( (worldPosition + cutoff - mapStart) / mapSize) * arrSize);

	float3 normalOffset = 0.f;

	float sum = 0.f;
	for (int x = indMin.x; x <= indMax.x; x++) {
		for (int y = indMin.y; y <= indMax.y; y++) {
			for (int z = indMin.z; z <= indMax.z; z++) {
				int i = Utils::to1D(int3(x,y,z), arrSize.x, arrSize.y);
				i = clamp(i, 0, WATER_ARR_SIZE - 1);
				
				float r = waterData[i];
				if (r > 0.f) {
					float3 waterPointWorldPos = float3(x,y,z) * cellWorldSize + mapStart;

					float3 dstV = waterPointWorldPos - worldPosition;
					float dstSqrd = dot(dstV, dstV);
					if (dstSqrd <= 0.09f * r) { // cutoff^2, reomve "* r" if CHARGE1 is used below
						// float charge = 1.f-(dstSqrd)*10.f; // CHARGE1: Disregards r
						float charge = 1.f-(dstSqrd * (1.f / r))*10.f; // CHARGE2: can handle changing blob sizes
						sum += charge * charge;
						normalOffset += normalize(-dstV);
						normalOffset += normalize(float3(x,y,z) - indMin);
					}
				}
			}
		}
	}

	if (sum > 0.8f) {
		float waterOpacity = clamp(sum / 1.f, 0.f, 0.8f);
		// payload.color = float4(sum / 10.0f, 0.f, 0.f, 1.0f);
		// return;

		// Shade as water

		metalness = lerp(metalness, 1.0f, waterOpacity);
		roughness = lerp(roughness, 0.01f, waterOpacity);
		ao 		  = lerp(ao, 0.5f, waterOpacity);
		albedo = lerp(albedo, albedo * 0.8f, waterOpacity);

		float height = 1.6f; // normal smoothness / water "height" apperance
		worldNormal += normalize(normalOffset) * clamp( ( -height*sum + 2.f ), 0.0f, 0.8f);
		worldNormal = normalize(worldNormal);
		// metalness = 1.0f;
		// roughness = 0.01f;
		// ao = 0.5f;
	}
	payload.color = pbrShade(worldPosition, worldNormal, -rayDir, albedo, metalness, roughness, ao, payload);
	// payload.color = float4(worldNormal * 0.5f + 0.5f, 1.0f);
	// payload.color = phongShade(worldPosition, worldNormal, albedo);
}