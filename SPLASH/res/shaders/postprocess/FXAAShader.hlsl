/*
 * Written by Emil Wahl 2017
 * Inspiration from http://blog.simonrodriguez.fr/articles/30-07-2016_implementing_fxaa.html
 * Ported to compute shader by Pirat 2019 ⛵
 */

Texture2D input : register(t0);
RWTexture2D<float4> output : register(u10) : SAIL_RGBA16_FLOAT;

SamplerState CSss : register(s2);

static const float EDGE_THRESHOLD_MIN = 0.0312;
static const float EDGE_THRESHOLD_MAX = 0.125;
static const float SUBPIXEL_QUALITY = 0.75;
static const float ITERATIONS = 12;
static const float QUALITY[12] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0 };

float rgbToLuma(float3 rgb) {
	return sqrt(dot(rgb, float3(0.299, 0.587, 0.114)));
}

cbuffer CSData : register(b0) {
    float textureSizeDifference;
    uint2 textureSize;
}

#define BLOCK_SIZE 256
[numthreads(BLOCK_SIZE, 1, 1)]
void CSMain(int3 groupThreadID : SV_GroupThreadID,
				int3 dispatchThreadID : SV_DispatchThreadID)
{
	if (dispatchThreadID.x > textureSize.x) {
        return;
    }

    float2 invTextureSize = 1.f / textureSize;

    float4 finalColor;
	bool isHorizontal = false;
	bool reached1 = false;
	bool reached2 = false;
	bool reachedBoth = false;
	bool isDirection1 = false;
	bool isLumaCenterSmaller = false;
	float luma1, luma2, distance1, distance2, lumaEnd, lumaEnd1, lumaEnd2;

	float3 colorCenter = input[dispatchThreadID.xy].rgb;

    // Luma at the current fragment
	float lumaCenter = rgbToLuma(colorCenter);

    // Luma at the four direct neighbours of the current fragment.
	float lumaDown = rgbToLuma(input[dispatchThreadID.xy + uint2(0, 1)].rgb);
	float lumaUp = rgbToLuma(input[dispatchThreadID.xy + uint2(0, -1)].rgb);
	float lumaLeft = rgbToLuma(input[dispatchThreadID.xy + uint2(-1, 0)].rgb);
	float lumaRight = rgbToLuma(input[dispatchThreadID.xy + uint2(1, 0)].rgb);
	
    // Find the maximum and minimum luma around the current fragment.
	float lumaMin = min(lumaCenter, min(min(lumaDown, lumaUp), min(lumaLeft, lumaRight)));
	float lumaMax = max(lumaCenter, max(max(lumaDown, lumaUp), max(lumaLeft, lumaRight)));

    // Compute the delta.
	float lumaRange = lumaMax - lumaMin;

    // If the luma variation is lower that a threshold (or if we are in a really dark area), we are not on an edge, don't perform any AA.
	if (lumaRange < max(EDGE_THRESHOLD_MIN, lumaMax * EDGE_THRESHOLD_MAX)) {
		finalColor = float4(colorCenter, 1.0f);
		output[dispatchThreadID.xy] = finalColor;
        return;
	}
    // Test edge detection
    // output[dispatchThreadID.xy] = float4(1.f, 0.f, 0.f, 1.0f);
    // return;

	float lumaDownLeft = rgbToLuma(input[dispatchThreadID.xy + uint2(-1, 1)].rgb);
	float lumaUpRight = rgbToLuma(input[dispatchThreadID.xy + uint2(1, -1)].rgb);
	float lumaUpLeft = rgbToLuma(input[dispatchThreadID.xy + uint2(-1, -1)].rgb);
	float lumaDownRight = rgbToLuma(input[dispatchThreadID.xy + uint2(1, 1)].rgb);
	
	float lumaDownUp = lumaDown + lumaUp;
	float lumaLeftRight = lumaLeft + lumaRight;
	
	float lumaLeftCorners = lumaDownLeft + lumaUpLeft;
	float lumaDownCorners = lumaDownLeft + lumaDownRight;
	float lumaRightCorners = lumaDownRight + lumaUpRight;
	float lumaUpCorners = lumaUpRight + lumaUpLeft;

	float edgeHorizontal = abs(-2.0f * lumaLeft + lumaLeftCorners) + abs(-2.0f * lumaCenter + lumaDownUp) * 2.0f + abs(-2.0f * lumaRight + lumaRightCorners);
	float edgeVertical = abs(-2.0f * lumaUp + lumaUpCorners) + abs(-2.0f *lumaCenter + lumaLeftRight) * 2.0f + abs(-2.0f * lumaDown + lumaDownCorners);

	if (edgeHorizontal > edgeVertical) {
		isHorizontal = true;
	}

    // Select the two neighboring texels lumas in the opposite direction to the local edge.
	if (isHorizontal) {
		luma1 = lumaDown;
		luma2 = lumaUp;
	}
	else {
		luma1 = lumaLeft;
		luma2 = lumaRight;
	}
    
    // Compute gradients in this direction.
	float gradient1 = luma1 - lumaCenter;
	float gradient2 = luma2 - lumaCenter;

    // Which direction is the steepest ?
    bool is1Steepest = abs(gradient1) >= abs(gradient2);

    // Gradient in the corresponding direction, normalized.
	float gradientScaled = 0.25f*max(abs(gradient1), abs(gradient2));

    // Choose the step size (one pixel) according to the edge direction.
    float stepLength = isHorizontal ? invTextureSize.y : invTextureSize.x;
    // float stepLength = 1.f; // Always one since we are sampling using integer coordinates (?)

    // Average luma in the correct direction.
    float lumaLocalAverage = 0.0;

	if (is1Steepest) {
        // Switch the direction
		stepLength = -stepLength;
		lumaLocalAverage = 0.5f*(luma1 + lumaCenter);
	}
	else {
		lumaLocalAverage = 0.5f*(luma2 + lumaCenter);
	}
	
    // Shift UV in the correct direction by half a pixel.
	float2 currentTexCoord = dispatchThreadID.xy * invTextureSize;
    float2 offset = 0.f;
	if (isHorizontal) {
		offset = float2(invTextureSize.x, 0.f);
		currentTexCoord.y += stepLength * 0.5f;
	}
	else {
		offset = float2(0.f, invTextureSize.y);
		currentTexCoord.x += stepLength * 0.5f;
	}

    // Compute UVs to explore on each side of the edge, orthogonally. The QUALITY allows us to step faster.
	float2 texCoord1 = currentTexCoord - offset;
	float2 texCoord2 = currentTexCoord + offset;

	for (int x = 0; x < ITERATIONS; x++) {
		if (!reached1) {
			lumaEnd1 = rgbToLuma(input.SampleLevel(CSss, texCoord1, 0).rgb);
			lumaEnd1 -= lumaLocalAverage;
		}
		if (!reached2) {
			lumaEnd2 = rgbToLuma(input.SampleLevel(CSss, texCoord2, 0).rgb);
			lumaEnd2 -= lumaLocalAverage;
		}
        // If the luma deltas at the current extremities is larger than the local gradient, we have reached the side of the edge.
        reached1 = abs(lumaEnd1) >= gradientScaled;
		reached2 = abs(lumaEnd2) >= gradientScaled;
        reachedBoth = reached1 && reached2;
		if (reachedBoth) {
			break;
		} else {
			if (!reached1) {
				texCoord1 -= offset * QUALITY[x];
			}
			if (!reached2) {
				texCoord2 += offset * QUALITY[x];
			}
		}
	}

    // Compute the distances to each extremity of the edge.
	if (isHorizontal) {
		distance1 = (dispatchThreadID.x * invTextureSize.x) - texCoord1.x;
		distance2 = texCoord2.x - (dispatchThreadID.x * invTextureSize.x);
	}
	else {
		distance1 = (dispatchThreadID.y * invTextureSize.y) - texCoord1.y;
		distance2 = texCoord2.y - (dispatchThreadID.y * invTextureSize.y);
	}

    // In which direction is the extremity of the edge closer ?
    isDirection1 = distance1 < distance2;
	float distanceFinal = min(distance1, distance2);

    // Length of the edge.
	float edgeThickness = distance1 + distance2;

    // UV offset: read in the direction of the closest side of the edge.
	float pixelOffset = -distanceFinal / edgeThickness + 0.5f; // Maybe multiply this by textureSize (?)
	
    // Is the luma at center smaller than the local average ?
    isLumaCenterSmaller = lumaCenter < lumaLocalAverage;
	
    // If the luma at center is smaller than at its neighbour, the delta luma at each end should be positive (same variation).
    // (in the direction of the closer side of the edge.)
    bool correctVariation = ((isDirection1 ? lumaEnd1 : lumaEnd2) < 0.0f) != isLumaCenterSmaller;
	
    // If the luma variation is incorrect, do not offset.
    float finalOffset = correctVariation ? pixelOffset : 0.0f;

    // Sub-pixel shifting
    // Full weighted average of the luma over the 3x3 neighborhood.
	float lumaAverage = (1.f/12.f) * (2.f * (lumaDownUp + lumaLeftRight) + lumaLeftCorners + lumaRightCorners);

	// Ratio of the delta between the global average and the center luma, over the luma range in the 3x3 neighborhood.
    float subPixelOffset1 = clamp(abs(lumaAverage - lumaCenter) / lumaRange, 0.f, 1.f);
    float subPixelOffset2 = (-2.f * subPixelOffset1 + 3.f) * subPixelOffset1 * subPixelOffset1;
    // Compute a sub-pixel offset based on this delta.
    float subPixelOffsetFinal = subPixelOffset2 * subPixelOffset2 * SUBPIXEL_QUALITY;

    // Pick the biggest of the two offsets.
    finalOffset = max(finalOffset, subPixelOffsetFinal);

    // Compute the final UV coordinates.
	float2 finalTexCoord = dispatchThreadID.xy * invTextureSize;
	if (isHorizontal) {
		finalTexCoord.y += finalOffset * stepLength;
	} else {
		finalTexCoord.x += finalOffset * stepLength;
	}

    // Read the color at the new UV coordinates, and use it.
	finalColor = input.SampleLevel(CSss, finalTexCoord, 0).rgba;

    output[dispatchThreadID.xy] = finalColor;
}
