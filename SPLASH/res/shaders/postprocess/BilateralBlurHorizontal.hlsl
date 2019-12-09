//=============================================================================
// Highly inspired by Frank Luna
//
// Performs a separable blur with a blur radius of 5.  
//=============================================================================
#include "BilateralBlurCommon.hlsl"

RWTexture2DArray<float4> inoutTexture : register(u10) : SAIL_IGNORE;

[numthreads(N, 1, 1)]
void CSMain(int3 groupThreadID : SV_GroupThreadID,
			int3 dispatchThreadID : SV_DispatchThreadID)
{
	//
	// Fill local thread storage to reduce bandwidth.  To blur 
	// N pixels, we will need to load N + 2*BlurRadius pixels
	// due to the blur radius.
	//

	float2 inputSize;
	uint numElements;
	inoutTexture.GetDimensions(inputSize.x, inputSize.y, numElements);

	for (uint lightIndex = 0; lightIndex < NUM_SHADOW_TEXTURES; lightIndex++) {
		// This thread group runs N threads.  To get the extra 2*BlurRadius pixels, 
		// have 2*BlurRadius threads sample an extra pixel.
		if(groupThreadID.x < blurRadius) {
			// Clamp out of bound samples that occur at image borders.
			int x = max(dispatchThreadID.x - blurRadius, 0);
			cache[lightIndex][groupThreadID.x] = inoutTexture[int3(x, dispatchThreadID.y, lightIndex)].rg;
		}
		if(groupThreadID.x >= N-blurRadius) {
			// Clamp out of bound samples that occur at image borders.
			int x = min(dispatchThreadID.x + blurRadius, inputSize.x-1);
			cache[lightIndex][groupThreadID.x+2*blurRadius] = inoutTexture[int3(x, dispatchThreadID.y, lightIndex)].rg;
		}

		// Clamp out of bound samples that occur at image borders.
		cache[lightIndex][groupThreadID.x+blurRadius] = inoutTexture[int3(min(dispatchThreadID.xy, inputSize.xy-1), lightIndex)].rg;

		// Wait for all threads to finish.
		GroupMemoryBarrierWithGroupSync();
		
		//
		// Now blur each pixel.
		//

		// float bZ = 1.0f / normpdf(0.0f, BSIGMA);
		float bZ = 1.0f;
		float2 Z = 0.f;
		float2 blurColor = 0.f;
		
		[unroll]
		for(int i = -blurRadius; i <= blurRadius; ++i) {
			int k = groupThreadID.x + blurRadius + i;
			
			float2 factor;
			factor.r = (1.f - abs(cache[lightIndex][k].r-cache[lightIndex][k - i].r)) * weights[i+blurRadius];
			factor.g = (1.f - abs(cache[lightIndex][k].g-cache[lightIndex][k - i].g)) * weights[i+blurRadius];

			Z += factor;
			blurColor += factor*cache[lightIndex][k];
		}
		
		inoutTexture[int3(dispatchThreadID.xy, lightIndex)] = float4(blurColor / Z, 0.f, 1.0f);
		// output[dispatchThreadID.xy] = float4(cache[lightIndex][groupThreadID.x + blurRadius], 0.f, 1.0f);
	}
}