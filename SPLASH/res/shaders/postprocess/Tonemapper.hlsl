Texture2D sourceTexture : register(t0);
RWTexture2D<float4> output : register(u10);

SamplerState CSss : register(s2);

float3 tonemap(float3 color) {
	// Gamma correction
    float3 output = color / (color + 1.0f);
    // Tone mapping using the Reinhard operator
    output = pow(output, 1.0f / 2.2f);
	return output;
}

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

    float3 finalColor = sourceTexture[dispatchThreadID.xy * textureSizeDifference].rgb;
	output[dispatchThreadID.xy] = float4(tonemap(finalColor), 1.0f);
}