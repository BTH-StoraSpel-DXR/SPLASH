Texture2D sourceTexture : register(t0);
Texture2D blendTexture : register(t1);
RWTexture2D<float4> output : register(u10) : SAIL_RGBA16_FLOAT;

SamplerState CSss : register(s2);

cbuffer CSData : register(b0) {
    float textureSizeDifference;
    uint2 textureSize;
    float blendFactor;
}

#define BLOCK_SIZE 256
[numthreads(BLOCK_SIZE, 1, 1)]
void CSMain(int3 groupThreadID : SV_GroupThreadID,
				int3 dispatchThreadID : SV_DispatchThreadID)
{
    if (dispatchThreadID.x > textureSize.x) {
        return;
    }
    float2 invTextureSize = 1.f / textureSize;

    float2 blendTexCoord = dispatchThreadID.xy * invTextureSize;
    float3 finalColor = sourceTexture[dispatchThreadID.xy].rgb + blendTexture.SampleLevel(CSss, blendTexCoord, 0).rgb * blendFactor;
	output[dispatchThreadID.xy] = float4(finalColor, 1.0f);
}