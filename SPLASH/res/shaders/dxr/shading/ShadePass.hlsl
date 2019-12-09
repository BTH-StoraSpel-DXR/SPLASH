#define HLSL
#include "../Common_hlsl_cpp.hlsl"
#include "../Utils.hlsl"

struct VSIn {
	float4 position : POSITION0;
};

struct PSIn {
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
};

PSIn VSMain(VSIn input) {
	PSIn output;

	input.position.w = 1.f;
	// input position is already in clip space coordinates
	output.position = input.position;
	output.texCoord.x = input.position.x / 2.f + 0.5f;
	output.texCoord.y = -input.position.y / 2.f + 0.5f;

	return output;
}

#include "PBR.hlsl"

cbuffer PSSceneCBuffer : register(b0) {
    float3 cameraPosition;
    float padding;
    PointlightInput pointLights[NUM_POINT_LIGHTS];
    SpotlightInput spotLights[NUM_SPOT_LIGHTS];
    IndexMap shadowTextureIndexMap[NUM_TOTAL_LIGHTS]; // Maps light indices to shadow texture indices
}

Texture2D<float4> albedoBounceOne : register(t0);
Texture2D<float4> albedoBounceTwo : register(t1);

Texture2D<float4> normalsBounceOne : register(t2);
Texture2D<float4> normalsBounceTwo : register(t3);

Texture2D<float4> metalnessRoughnessAoBounceOne : register(t4);
Texture2D<float4> metalnessRoughnessAoBounceTwo : register(t5);

Texture2DArray<float2> shadows : register(t6);

Texture2D<float4> worldPositionsOne : register(t7);
Texture2D<float4> worldPositionsTwo : register(t8);

Texture2D<float4> brdfLUT : register(t9);

SamplerState PSss : register(s0);

struct Outputs {
	float4 shaded   : SV_Target0;
	float4 bloom    : SV_Target1;
};
Outputs PSMain(PSIn input) {
    Outputs output;

    PBRPixel pixelOne;
    PBRPixel pixelTwo;

    pixelOne.worldNormal = normalsBounceOne.Sample(PSss, input.texCoord).xyz * 2.f - 1.f;
    pixelTwo.worldNormal = normalsBounceTwo.Sample(PSss, input.texCoord).xyz * 2.f - 1.f;
    
    pixelOne.worldPosition = worldPositionsOne.Sample(PSss, input.texCoord).xyz;
    pixelTwo.worldPosition = worldPositionsTwo.Sample(PSss, input.texCoord).xyz;
    
    pixelOne.albedo = albedoBounceOne.Sample(PSss, input.texCoord).rgb;
    pixelTwo.albedo = albedoBounceTwo.Sample(PSss, input.texCoord).rgb;

    float4 metalnessRoughnessAoOne = metalnessRoughnessAoBounceOne.Sample(PSss, input.texCoord);
    float4 metalnessRoughnessAoTwo = metalnessRoughnessAoBounceTwo.Sample(PSss, input.texCoord);
    pixelOne.metalness = metalnessRoughnessAoOne.r;
    pixelTwo.metalness = metalnessRoughnessAoTwo.r;
    pixelOne.roughness = metalnessRoughnessAoOne.g;
    pixelTwo.roughness = metalnessRoughnessAoTwo.g;
    pixelOne.ao = metalnessRoughnessAoOne.b;
    pixelTwo.ao = metalnessRoughnessAoTwo.b;
    pixelOne.emissiveness = metalnessRoughnessAoOne.a;
    pixelTwo.emissiveness = metalnessRoughnessAoTwo.a;

    float shadowOne[NUM_TOTAL_LIGHTS];
    float shadowTwo[NUM_TOTAL_LIGHTS];
    for (uint i = 0; i < NUM_SHADOW_TEXTURES; i++) {
        float2 shadowAmount = shadows.Sample(PSss, float3(input.texCoord, i)).rg; // z parameter in texCoords is the array index
        shadowOne[i] = shadowAmount.r;
        shadowTwo[i] = shadowAmount.g;
    }
    // Fill out the rest of the array with zeros if NUM_SHADOW_TEXTURES < NUM_TOTAL_LIGHTS
    for (uint j = NUM_SHADOW_TEXTURES; j < NUM_TOTAL_LIGHTS; j++) {
        shadowOne[j] = 0.f;
        shadowTwo[j] = 0.f;
    }
    
    pixelOne.invViewDir = cameraPosition - pixelOne.worldPosition;
    pixelTwo.invViewDir = pixelOne.worldPosition - pixelTwo.worldPosition;

    PBRScene scene;
    // Shade the reflection
    scene.pointLights = pointLights;
    scene.spotLights = spotLights;
    scene.brdfLUT = brdfLUT;
    scene.sampler = PSss;
    scene.shadow = shadowTwo;
    scene.shadowTextureIndexMap = shadowTextureIndexMap;
	float4 secondBounceColor = pbrShade(scene, pixelTwo, -1.f);

    // Shade the first hit
    scene.shadow = shadowOne;
    float4 outputColor = pbrShade(scene, pixelOne, secondBounceColor.rgb);
    // return outputColor;
    output.shaded = outputColor;

    // Write bloom pass input
    output.bloom = float4((length(outputColor.rgb) > 1.0f) ? clamp(outputColor.rgb, 0.f, 3.f) : 0.f, 1.0f);
    
    // Debug stuff
    // return secondBounceColor;
    // return float4(albedoOne, 1.0f);
    // return float4(pixelOne.worldNormal, 1.0f);
    // return float4(shadowTwo[0], 0.f, 0.f, 1.0f);
    // return float4(shadowAmount.x, 0.f, 0.f, 1.0f) * 0.5 + pbrShade(worldPositionOne, worldNormalOne, invViewDirOne, albedoOne, metalnessOne, roughnessOne, aoOne, shadowOne, secondBounceColor) * 0.5;

    return output;
}

