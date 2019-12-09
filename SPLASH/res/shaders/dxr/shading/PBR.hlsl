#define HLSL
#include "../Common_hlsl_cpp.hlsl"

#define RENDER_SHADOWS_SEPARATELY

float3 fresnelSchlick(float cosTheta, float3 F0) {
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
} 
float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness) {
    return F0 + (max(1.0f - roughness, F0) - F0) * pow(1.0f - cosTheta, 5.0f);
}  

float DistributionGGX(float3 N, float3 H, float roughness) {
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float num   = NdotV;
    float denom = NdotV * (1.0f - k) + k;
	
    return num / denom;
}
float GeometrySmith(float3 N, float3 V, float3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

inline float3 shadeWithLight(PointlightInput light, float3 worldPosition, float3 N, float3 V, float3 F0, float3 albedo, float metalness, float roughness, float shadowAmount) {
    float3 L = normalize(light.position - worldPosition);
    float3 H = normalize(V + L);
    float distance = length(light.position - worldPosition);

    // Standard attenuation
    // float attenuation = 1.f / (light.attConstant + light.attLinear * distance + light.attQuadratic * distance * distance);
    // Tweaked UE4 attenuation
    float lightRadius = light.reachRadius;
    float attenuation = pow(saturate(1.f - pow(distance/lightRadius, 4.f)), 2.f) / (distance * distance + 1.f);
    attenuation *= 8.f;
    
    float3 radiance   = light.color * attenuation;

    float3 F  = fresnelSchlick(max(dot(H, V), 0.0f), F0);

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);

    // Calculate the Cook-Torrance BDRF
    float3 numerator    = NDF * G * F;
    float  denominator  = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f);
    float3 specular     = numerator / max(denominator, 0.001f); // constrain the denominator to 0.001 to prevent a divide by zero in case any dot product ends up 0.0
    specular *= attenuation;

    // The fresnel value directly corresponds to the specular contribution
    float3 kS = F;
    // The rest is the diffuse contribution (energy conserving)
    float3 kD = 1.0f - kS;
    // Because metallic surfaces don't refract light and thus have no diffuse reflections we enforce this property by nullifying kD if the surface is metallic
    kD *= 1.0f - metalness;

    // Calculate the light's outgoing reflectance value
    float NdotL = max(dot(N, L), 0.0f);
    return (kD * albedo / PI + specular) * radiance * NdotL * shadowAmount;
}

// PBR input structs
struct IndexMap {
    int index;
    float3 padding;
};
struct PBRScene {
    PointlightInput pointLights[NUM_POINT_LIGHTS];
    SpotlightInput spotLights[NUM_SPOT_LIGHTS];
    Texture2D<float4> brdfLUT;
    SamplerState sampler;
    // Only used for soft shadows
    float shadow[NUM_TOTAL_LIGHTS];
    IndexMap shadowTextureIndexMap[NUM_TOTAL_LIGHTS]; // Maps light indices to shadow texture indices
};
struct PBRPixel {
    float3 worldPosition;
    float3 worldNormal;
    float3 invViewDir;
    float3 albedo;
    float emissiveness;
    float metalness;
    float roughness;
    float ao;
};

float4 pbrShade(PBRScene scene, PBRPixel pixel, float3 reflectionColor) {
    float3 N = normalize(pixel.worldNormal); 
    float3 V = normalize(pixel.invViewDir);

    float3 F0 = 0.04f;
    F0        = lerp(F0, pixel.albedo, pixel.metalness);

    // Reflectance equation
    float3 Lo = 0.0f;
    // Add emissive materials
	Lo += pixel.albedo * 2.2 * pixel.emissiveness;

    float totalShadowAmount = 0.f;
	uint numLights = 0;
    
    // Do lighting from all light sources
    {
        // Point lights
        // [unroll]
        PointlightInput p;
        for(int i = 0; i < NUM_POINT_LIGHTS; i++) {
            p = scene.pointLights[i];
            // Ignore point light if color is black
            if (all(p.color == 0.0f)) {
                continue;
            }
            float distance = length(p.position - pixel.worldPosition);
            // Ignore lights that are too far away
            if (distance > p.reachRadius) {
                continue;
            }
#ifdef RAYTRACER_HARD_SHADOWS
    float3 direction = normalize(p.position - pixel.worldPosition);
    // Cast hard shadow ray, 1spp
    float shadowAmount = 1.f - (float)Utils::rayHitAnything(pixel.worldPosition, pixel.worldNormal, direction, distance);
#else
            int shadowTextureIndex = scene.shadowTextureIndexMap[i].index;
            if (shadowTextureIndex == -1) {
                // No shadow texture is bound to this light, skip it
                continue;
            }
            float shadowAmount = scene.shadow[shadowTextureIndex];
#endif
            // float shadowAmount = 1.f;
            numLights++;
            // Dont add light to pixels that are in complete shadow
            if (shadowAmount == 0.f) {
                continue;
            }
            totalShadowAmount += shadowAmount;

            Lo += shadeWithLight(p, pixel.worldPosition, N, V, F0, pixel.albedo, pixel.metalness, pixel.roughness, shadowAmount);
        }
        // Spotlights
        // [unroll]
        SpotlightInput s;
		for (int j = 0; j < NUM_SPOT_LIGHTS; j++) {
            s = scene.spotLights[j];
			// Ignore point light if color is black
			if (all(s.color == 0.0f) || s.angle == 0) {
				continue;
			}
			float3 L = normalize(s.position - pixel.worldPosition);
			float angle = dot(L, normalize(s.direction));
            // Ignore if angle is outside light
            // abs is used to make spotlights bidirectional
			if (abs(angle) <= s.angle) {
				continue;
			}
            float distance = length(s.position - pixel.worldPosition);
            // Ignore lights that are too far away
            if (distance > s.reachRadius) {
                continue;
            }
#ifdef RAYTRACER_HARD_SHADOWS
    float3 direction = normalize(s.position - pixel.worldPosition);
    // Cast hard shadow ray, 1spp
    float shadowAmount = 1.f - (float)Utils::rayHitAnything(pixel.worldPosition, pixel.worldNormal, direction, distance);
#else
            int shadowTextureIndex = scene.shadowTextureIndexMap[j + NUM_POINT_LIGHTS].index;
            if (shadowTextureIndex == -1) {
                // No shadow texture is bound to this light, skip it
                continue;
            }
            float shadowAmount = scene.shadow[shadowTextureIndex];
#endif


			numLights++;
			// Dont add light to pixels that are in complete shadow
            if (shadowAmount == 0.f) {
                continue;
            }
            totalShadowAmount += shadowAmount;

            PointlightInput castedP = (PointlightInput)s; // Not doing the cast on a separate line breaks shader compilation
            Lo += shadeWithLight(castedP, pixel.worldPosition, N, V, F0, pixel.albedo, pixel.metalness, pixel.roughness, shadowAmount);
		}
    }

    // Lerp AO depending on shadow
	// This fixes water being visible in darkness
	totalShadowAmount /= max(numLights, 1);
	totalShadowAmount = (totalShadowAmount * totalShadowAmount);
	// pixel.ao = lerp(0.f, pixel.ao, totalShadowAmount);

    // Use this when we have cube maps for irradiance, pre filtered reflections and brdfLUT
    float3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0f), F0, pixel.roughness);

    float3 kS = F;
    float3 kD = 1.0f - kS;
    kD *= 1.0f - pixel.metalness;	  
    
    // Assume constant irradiance since game is played indoors with no or few indirect light sources
    // This could be read from a skymap or irradiance probe
    float3 irradiance = 0.0f;
    float3 diffuse    = irradiance * pixel.albedo;
    
    float3 R = reflect(-V, N);  

    // Reflection
    float3 ambient = 0.f;
    if (all(reflectionColor == -1.f)) {
        // Parse negative reflection as no color
        // Use color from only direct light and irradiance
        ambient = irradiance * pixel.albedo * pixel.ao;
    } else {
        // Use reflectionColor parameter
        float3 prefilteredColor = reflectionColor;

        // Assume roughness = 0
        // This is a very rough approximation used because ray traced reflections are always perfect
        // and blurring them to make accurate PBR calculations is very expensive in real-time
        float2 envBRDF = scene.brdfLUT.SampleLevel(scene.sampler, float2(max(dot(N, V), 0.0f), pixel.roughness), 0).rg;
        float3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);
        ambient = (kD * diffuse + specular) * pixel.ao;
    }

    // Add the (improvised) ambient term to get the final color of the pixel
    float3 color = ambient + Lo;

    return float4(color, 1.0f);
}