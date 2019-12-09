#ifndef __COMMON_HLSL__
#define __COMMON_HLSL__

#define NUM_POINT_LIGHTS 8

struct PointLight {
	float3 color;
	float distanceToLight;
	float3 fragToLight;
    float reachRadius;
};

struct DirectionalLight {
	float3 color;
	float3 direction;
};

struct LightList {
	DirectionalLight dirLight;
    PointLight pointLights[NUM_POINT_LIGHTS];
};

struct Material {
	float4 modelColor;
	float metalnessScale;
	float roughnessScale;
	float aoScale;
    float padding;
	bool hasAlbedoTexture; // TODO : Pack flags
	bool hasNormalTexture;
	bool hasMetalnessRoughnessAOTexture;
};

#endif // __COMMON_HLSL__