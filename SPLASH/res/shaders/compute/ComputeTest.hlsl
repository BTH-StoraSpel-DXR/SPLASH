Texture2D input : register(t0);
RWTexture2D<float4> output : register(u0);
SamplerState ss : register(s0);

[numthreads(1, 1, 1)]
void CSMain(int3 groupThreadID : SV_GroupThreadID, int3 dispatchThreadID : SV_DispatchThreadID) {
    output[dispatchThreadID.xy] = input[dispatchThreadID.xy];
}
