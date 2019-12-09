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
    float4 clipPos : POS0;
    float4 clipPosLastFrame : POS1;
    float3 normal : NORMAL0;
    float2 texCoords : TEXCOORD0;
    float3x3 tbn : TBN;
};

cbuffer VSSystemCBuffer : register(b0) {
    matrix sys_mWorld;
    matrix sys_mView;
    matrix sys_mProj;
    matrix sys_mWVPLastFrame;
}

cbuffer PSSystemCBuffer : register(b1) {
    Material sys_material_pbr;
}
cbuffer PSSystemCBuffer_2 : register(b1, space1) {
	float3 teamColor;
}

PSIn VSMain(VSIn input) {
    PSIn output;

    matrix wv = mul(sys_mView, sys_mWorld);

	output.texCoords = input.texCoords;
	input.position.w = 1.f;
	// Convert position into clip space
	output.position = mul(wv, input.position);
    output.position = mul(sys_mProj, output.position);
    output.clipPos = output.position;
    output.clipPosLastFrame = mul(sys_mWVPLastFrame, input.position);
    
	// Convert normal into world space and normalize
	output.normal = mul((float3x3) sys_mWorld, input.normal);
	output.normal = normalize(output.normal);

	// Create TBN matrix to go from tangent space to world space
	output.tbn = float3x3(
	  normalize(mul((float3x3) sys_mWorld, input.tangent)),
	  normalize(mul((float3x3) sys_mWorld, input.bitangent)),
	  output.normal
	);

	return output;
}

// TODO: check if it is worth the extra VRAM to write diffuse and specular gbuffers instead of sampling them in the raytracing shaders
//       The advantage is that mip map level can be calculated automatically here
//      
Texture2D sys_texAlbedo                 : register(t0);
Texture2D sys_texNormal                 : register(t1);
Texture2D sys_texMetalnessRoughnessAO   : register(t2);
SamplerState PSss;

struct GBuffers {
	float4 normal               : SV_Target0;
	float4 albedo               : SV_Target1;
	float4 metalnessRoughnessAO : SV_Target2;
    float2 motionVector         : SV_Target3;
};

GBuffers PSMain(PSIn input) {
	GBuffers gbuffers;

	gbuffers.normal = float4(normalize(input.normal) / 2.f + .5f, 1.f);
    if (sys_material_pbr.hasNormalTexture) {
        float3 normalSample = sys_texNormal.Sample(PSss, input.texCoords).rgb;
        normalSample.y = 1.0f - normalSample.y;
        gbuffers.normal = float4(mul(normalize(normalSample * 2.f - 1.f), input.tbn) / 2.f + .5f, 1.0f);
    }

    gbuffers.albedo = sys_material_pbr.modelColor;
	if (sys_material_pbr.hasAlbedoTexture) {
		float4 albedo = sys_texAlbedo.Sample(PSss, input.texCoords);

		if (albedo.a < 1.0f) {
		 	float f = 1 - albedo.a;
		 	gbuffers.albedo = float4(gbuffers.albedo.rgb * (1 - f) + teamColor * f, albedo.a);
		} else {
			gbuffers.albedo *= albedo;
		}
	}

    gbuffers.metalnessRoughnessAO = float4(sys_material_pbr.metalnessScale, sys_material_pbr.roughnessScale, sys_material_pbr.aoScale, 1.0f);
	if (sys_material_pbr.hasMetalnessRoughnessAOTexture)
		gbuffers.metalnessRoughnessAO *= sys_texMetalnessRoughnessAO.Sample(PSss, input.texCoords);

    float2 position = input.clipPos.xy / input.clipPos.w * 0.5f + 0.5f;
    float2 positionLastFrame = input.clipPosLastFrame.xy / input.clipPosLastFrame.w * 0.5f + 0.5f;
    gbuffers.motionVector = (position - positionLastFrame) * 0.5f + 0.5f;

    return gbuffers;
}
