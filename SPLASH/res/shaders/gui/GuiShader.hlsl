
struct VSIn {
	float2 position : POSITION0;
	float2 texCoords : TEXCOORD0;
};

struct PSIn {
	float4 position : SV_Position;
	float2 texCoords : TEXCOORD0;
};

Texture2D<float4> sys_texAlbedo 	: register(t0);
SamplerState PSss 					: register(s0);

PSIn VSMain(VSIn input) {
	PSIn output;

	output.position = float4(input.position.xy, 0.f, 1.f);
	output.texCoords = input.texCoords;

	return output;
}

float4 PSMain(PSIn input) : SV_Target0 {
	return float4(1.f, 1.f, 1.f, 0.5f);
	//return sys_texAlbedo.Sample(PSss, float2(input.texCoords.x, input.texCoords.y));
}

