// Kernel from http://dev.theomader.com/gaussian-kernel-calculator/
//    0.382925f, 0.24173f, 0.060598f, 0.005977f, 0.000229f
// Offsets and weights calculated using formula provided by Daniel Rákos
// https://web.archive.org/web/20170926040515/http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
static const float offset[3] =
{
    0.f, 1.20043793f, 3.03689977f
};
static const float weight[3] =
{
    0.382925f, 0.302328f, 0.006206f
};

float getBrightness(float3 color) {
    return dot(color, float3(0.2126, 0.7152, 0.0722));
}

struct VSIn {
    float4 position : POSITION0;
};

struct PSIn {
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
};

cbuffer CBuffer : register(b0) {
    float invWindowWidth;
    float invWindowHeight;
}

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
Texture2D depthTex : register(t1);
SamplerState ss : register(s0);

#define DEPTH_THRESHOLD 0.001f

float4 PSHorizontal(PSIn input) : SV_Target0 {

    float centerDepth = depthTex.Sample(ss, input.texCoord).r;
    float4 color = tex.Sample(ss, input.texCoord) * weight[0];

    for (int x = 1; x < 3; x++) {
        float2 coord = input.texCoord + float2(offset[x] * invWindowWidth, 0.f);
        if (abs(depthTex.Sample(ss, coord).r - centerDepth) < DEPTH_THRESHOLD) {
            color += tex.Sample(ss, coord) * weight[x];
        }
        coord = input.texCoord - float2(offset[x] * invWindowWidth, 0.f);
        if (abs(depthTex.Sample(ss, coord).r - centerDepth) < DEPTH_THRESHOLD) {
            color += tex.Sample(ss, coord) * weight[x];
        }
    }
    
    return color;

}

float4 PSVertical(PSIn input) : SV_Target0 {

    float centerDepth = depthTex.Sample(ss, input.texCoord).r;
    float4 color = tex.Sample(ss, input.texCoord) * weight[0];
    float centerColor = color;

    for (int y = 1; y < 3; y++) {
        float2 coord = input.texCoord + float2(0.f, offset[y] * invWindowHeight);
        if (abs(depthTex.Sample(ss, coord).r - centerDepth) < DEPTH_THRESHOLD) {
            color += tex.Sample(ss, coord) * weight[y];
        } else {
            color += centerColor * weight[y];
        }
        coord = input.texCoord - float2(0.f, offset[y] * invWindowHeight);
        if (abs(depthTex.Sample(ss, coord).r - centerDepth) < DEPTH_THRESHOLD) {
            color += tex.Sample(ss, coord) * weight[y];
        } else {
            color += centerColor * weight[y];
        }
    }
    
    return color;

}

