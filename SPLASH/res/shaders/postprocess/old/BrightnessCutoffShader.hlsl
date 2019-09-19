//cbuffer CBuffer : register(b0) {
//  float cutoff;
//}

struct PSIn {
  float4 position : SV_Position;
  float2 texCoord : TEXCOORD0;
};

Texture2D tex : register(t0);
SamplerState ss : register(s0);

float4 PSMain(PSIn input) : SV_Target0 {

  float4 color = tex.Sample(ss, input.texCoord);
  return float4(color.rgb * dot(color.rgb, float3(0.333, 0.333, 0.333)), 1.f);

}
