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

Texture2D tex : register(t0);
SamplerState ss : register(s0);

float4 PSMain(PSIn input) : SV_Target0 {

  float4 color = tex.Sample(ss, input.texCoord);
  float multipier = 1.f / max(max(color.r, max(color.g, color.b)), 1.f);
  //multipier = 1.f;
  //float n = snoise(input.texCoord * 500.f) * 0.03f;
  //float3 noise = float3(n, n, n);
  //return float4(color.rgb * multipier + noise, color.a);
  return float4(color.rgb * multipier, color.a);

}
