// #define WATER_DEBUG
void getWaterMaterialOnSurface(inout float3 albedo, inout float metalness, inout float roughness, inout float ao, inout float3 worldNormal, float3 worldPosition) {
	// =================================================
	//  Render pixel as water if close to a water point
	// =================================================

	float3 arrSize = (float3)CB_SceneData.waterArraySize;
	arrSize.x = max(1, arrSize.x);
	arrSize.y = max(1, arrSize.y);
	arrSize.z = max(1, arrSize.z);
	float waterArrSize = arrSize.x * arrSize.y * arrSize.z;
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
				i = clamp(i, 0, floor(waterArrSize) - 1);
				uint packedR = waterData[i];

				uint start = (x == indMin.x) ? floor(((floatIndMin.x - floor(floatIndMin.x)) * 4.f) % 4) : 0;
				uint end = (x == indMax.x) ? floor(((floatIndMax.x - floor(floatIndMax.x)) * 4.f) % 4) : 3;

				// [unroll]
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
		albedo = float3(1.0f, 0.f, 0.f);
		return;
#endif
		float waterOpacity = clamp(sum / 1.f, 0.f, 0.8f);

		// Shade as water

		metalness = lerp(metalness, 1.0f, 			waterOpacity);
		roughness = lerp(roughness, 0.01f, 			waterOpacity);
		ao 		  = lerp(ao, 		0.5f, 			waterOpacity);
		albedo 	  = lerp(albedo, 	albedo * 0.8f,  waterOpacity);

		float height = 1.6f; // normal smoothness / water "height" apperance
		worldNormal += normalize(normalOffset) * clamp( ( -height*sum + 2.f ), 0.0f, 0.8f);
		worldNormal = normalize(worldNormal);
	}
}