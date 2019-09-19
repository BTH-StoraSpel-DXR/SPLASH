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

Texture2D colorTex : register(t0);
Texture2D blendTex : register(t1);
SamplerState ss : register(s0);

cbuffer cb : register(b0) {
	float blendFactor;
}

float4 PSMain(PSIn input) : SV_Target0 {
	return colorTex.Sample(ss, input.texCoord) + blendTex.Sample(ss, input.texCoord) * blendFactor;
}

