#include "../Common.hlsl"

struct VSIn {
	float4 position : POSITION0;
	float2 texCoords : TEXCOORD0;
	float3 normal : NORMAL0;
	float3 tangent : TANGENT0;
	float3 bitangent : BINORMAL0;
};

struct PSIn {
    float4 position : SV_Position;
    float3 normal : NORMAL0;
    float2 texCoords : TEXCOORD0;
};

cbuffer VSSystemCBuffer : register(b0) {
    matrix sys_mWorld;
    matrix sys_mView;
    matrix sys_mProj;
}

cbuffer PSSystemCBuffer : register(b1) {
    Material sys_material_pbr;
}

PSIn VSMain(VSIn input) {
    PSIn output;
    matrix wv = mul(sys_mView, sys_mWorld);
	output.texCoords = input.texCoords;
	input.position.w = 1.f;
	output.position = mul(wv, input.position);
	// Convert position into projection space
    output.position = mul(sys_mProj, output.position);
    
	// Convert normal into world space and normalize
	output.normal = mul((float3x3) sys_mWorld, input.normal);
	output.normal = normalize(output.normal);

	return output;
}

Texture2D sys_texAlbedo : register(t0);
SamplerState PSss;

float4 PSMain(PSIn input) : SV_Target0 {
    float4 color = sys_material_pbr.modelColor;
	if (sys_material_pbr.hasAlbedoTexture) {
		float4 albedo = sys_texAlbedo.SampleLevel(PSss, input.texCoords, 0);
        color *= albedo;
	}

    return color;
}
