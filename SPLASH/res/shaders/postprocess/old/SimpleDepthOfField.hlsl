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
Texture2D blurTex : register(t1);
Texture2D depthTex : register(t2);
SamplerState ss : register(s0);

cbuffer cb : register(b0) {
	float zNear;
	float zFar;
	float focalDistance;
	float focalWidth;
}

float4 PSMain(PSIn input) : SV_Target0 {
	float z = depthTex.Sample(ss, input.texCoord).r;
	float pixelDepth = zNear*zFar / (zFar - z*(zFar-zNear));

	//float focalDistance = 5.f;
	//float focalDepth = 10.f;
  	

	//float blendFactor = saturate(pow(pixelDepth - 5, 2) / zFar + zNear);
	float blendFactor = smoothstep(0.f, focalWidth, abs(focalDistance - pixelDepth));
	//float blendFactor = abs(smoothstep(-focalWidth * 0.5f, focalWidth * 0.5f, focalDistance - pixelDepth));


	return lerp(colorTex.Sample(ss, input.texCoord), blurTex.Sample(ss, input.texCoord), blendFactor);
	//return float4(blendFactor, blendFactor, blendFactor, 1.f);
}

