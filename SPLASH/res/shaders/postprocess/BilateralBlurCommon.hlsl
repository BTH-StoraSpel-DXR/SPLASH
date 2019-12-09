static float weights[21] = {
	0.028174f, 0.032676f, 0.037311f, 0.041944f, 0.046421f, 0.050582f, 0.054261f, 0.057307f, 0.059587f, 0.060998f, 0.061476f, 0.060998f, 0.059587f, 0.057307f, 0.054261f, 0.050582f, 0.046421f, 0.041944f, 0.037311f, 0.032676f, 0.028174f
};

static const int blurRadius = 10;

// static float weights[11] = {
// 	0.05f, 0.05f, 0.1f, 0.1f, 0.1f, 0.2f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f,
// };

// static const int blurRadius = 5;

#define HLSL
#include "../dxr/Common_hlsl_cpp.hlsl"

#define N 256
#define cacheSize (N + 2 * blurRadius)
groupshared float2 cache[NUM_SHADOW_TEXTURES][cacheSize];

#define BSIGMA 1.0

float normpdf3(in float3 v, in float sigma) {
	return 0.39894f * exp(-0.5f * dot(v,v) / (sigma * sigma)) / sigma;
}
