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

/*
 * Written by Emil Wahl 2017
 */

static const float EDGE_THRESHOLD_MIN = 0.0312;
static const float EDGE_THRESHOLD_MAX = 0.125;
static const float SUBPIXEL_QUALITY = 0.75;
static const float ITERATIONS = 12;
static const float QUALITY[12] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0 };

float rgbToLuma(float3 rgb) {
	return sqrt(dot(rgb, float3(0.299, 0.587, 0.114)));
}

cbuffer PSWindowSize : register(b0) {
	float windowWidth;
	float windowHeight;
}

Texture2D tex : register(t0);
SamplerState PSss : register(s0);


float4 PSMain(PSIn input) : SV_Target0
{
	float4 finalColor;
	bool isHorizontal = false;
	bool reached1 = false;
	bool reached2 = false;
	bool reachedBoth = false;
	bool is1Steepest = false;
	bool isDirection1 = false;
	bool isLumaCenterSmaller = false;
	bool correctVariation = false;
	float luma1, luma2, stepLength, lumaLocalAverage, distance1, distance2, lumaEnd, lumaEnd1, lumaEnd2;
	float2 offset;

	float2 pixelSize = float2(1.0f / windowWidth, 1.0f / windowHeight);
	
	float3 colorCenter = tex.Sample(PSss, input.texCoord).rgb;

	float lumaCenter = rgbToLuma(colorCenter);

	float lumaDown = rgbToLuma(tex.Sample(PSss, input.texCoord + float2(0.0f, pixelSize.y)).rgb);
	float lumaUp = rgbToLuma(tex.Sample(PSss, input.texCoord + float2(0.0f, -pixelSize.y)).rgb);
	float lumaLeft = rgbToLuma(tex.Sample(PSss, input.texCoord + float2(-pixelSize.x, 0.0f)).rgb);
	float lumaRight = rgbToLuma(tex.Sample(PSss, input.texCoord + float2(pixelSize.x, 0.0f)).rgb);
	
	float lumaMin = min(lumaCenter, min(min(lumaDown, lumaUp), min(lumaLeft, lumaRight)));
	float lumaMax = max(lumaCenter, max(max(lumaDown, lumaUp), max(lumaLeft, lumaRight)));

	float lumaRange = lumaMax - lumaMin;

	
	if (lumaRange < max(EDGE_THRESHOLD_MIN, lumaMax * EDGE_THRESHOLD_MAX)) {
		finalColor = tex.Sample(PSss, input.texCoord).rgba;
		return finalColor;
	}

	float lumaDownLeft = rgbToLuma(tex.Sample(PSss, input.texCoord + float2(-pixelSize.x, pixelSize.y)).rgb);
	float lumaUpLeft = rgbToLuma(tex.Sample(PSss, input.texCoord + float2(-pixelSize.x, -pixelSize.y)).rgb);
	float lumaDownRight = rgbToLuma(tex.Sample(PSss, input.texCoord + float2(pixelSize.x, pixelSize.y)).rgb);
	float lumaUpRight = rgbToLuma(tex.Sample(PSss, input.texCoord + float2(pixelSize.x, -pixelSize.y)).rgb);
	
	float lumaDownUp = lumaDown + lumaUp;
	float lumaLeftRight = lumaLeft + lumaRight;
	
	float lumaLeftCorners = lumaDownLeft + lumaUpLeft;
	float lumaDownCorners = lumaDownLeft + lumaDownRight;
	float lumaRightCorners = lumaDownRight + lumaUpRight;
	float lumaUpCorners = lumaUpRight + lumaUpLeft;

	float edgeHorizontal = abs(-2.0 * lumaLeft + lumaLeftCorners) + abs(-2.0 * lumaCenter + lumaDownUp) * 2.0 + abs(-2.0 * lumaRight + lumaRightCorners);
	float edgeVertical = abs(-2.0 * lumaUp + lumaUpCorners) + abs(-2.0 *lumaCenter + lumaLeftRight) * 2.0 + abs(-2.0 * lumaDown + lumaDownCorners);

	if (edgeHorizontal > edgeVertical) {
		isHorizontal = true;
	}

	if (isHorizontal) {
		luma1 = lumaUp;
		luma2 = lumaDown;
	}
	else {
		luma1 = lumaLeft;
		luma2 = lumaRight;
	}
	
	float gradient1 = luma1 - lumaCenter;
	float gradient2 = luma2 - lumaCenter;

	if (abs(gradient1) >= abs(gradient2)) {
		is1Steepest = true;
	}

	float gradientScaled = 0.25*max(abs(gradient1), abs(gradient2));


	if (isHorizontal) {
		stepLength = pixelSize.y;
	}
	else {
		stepLength = pixelSize.x;
	}

	if (is1Steepest) {
		stepLength = -stepLength;
		lumaLocalAverage = 0.5*(luma1 + lumaCenter);
	}
	else {
		lumaLocalAverage = 0.5*(luma2 + lumaCenter);
	}
	
	float2 currentTexCoord = input.texCoord;

	if (isHorizontal) {
		offset = float2((1.f/windowWidth), 0.0);
		currentTexCoord.y += stepLength * 0.5;
	}
	else {
		offset = float2(0.0, (1.f/windowHeight));
		currentTexCoord.x += stepLength * 0.5;
	}

	float2 texCoord1 = currentTexCoord - offset;
	float2 texCoord2 = currentTexCoord + offset;

	for (int x = 0; x < ITERATIONS; x++) {
		if (!reached1) {
			lumaEnd1 = rgbToLuma(tex.Sample(PSss, texCoord1).rgb);
			lumaEnd1 -= lumaLocalAverage;
		}
		if (!reached2) {
			lumaEnd2 = rgbToLuma(tex.Sample(PSss, texCoord2).rgb);
			lumaEnd2 -= lumaLocalAverage;
		}
		if (abs(lumaEnd1) >= gradientScaled) {
			reached1 = true;
		}
		if (abs(lumaEnd2) >= gradientScaled) {
			reached2 = true;
		}
		if (reached1 && reached2) {
			reachedBoth = true;
		}
		if (reachedBoth) {
			x = ITERATIONS;
		}
		else {
			if (!reached1) {
				texCoord1 -= offset *QUALITY[x];
			}
			if (!reached2) {
				texCoord2 += offset *QUALITY[x];
			}
		}
	}

	if (isHorizontal) {
		distance1 = input.texCoord.x - texCoord1.x;
		distance2 = texCoord2.x - input.texCoord.x;
	}
	else {
		distance1 = input.texCoord.y - texCoord1.y;
		distance2 = texCoord2.y - input.texCoord.y;
	}

	if (distance1 < distance2) {
		isDirection1 = true;
	}
	
	float distanceFinal = min(distance1, distance2);
	float edgeThickness = (distance1 + distance2);
	float pixelOffset = -distanceFinal / edgeThickness + 0.5;
	
	if (lumaCenter < lumaLocalAverage) {
		isLumaCenterSmaller = true;
	}
	
	if (isDirection1) {
		lumaEnd = lumaEnd1;
	}
	else {
		lumaEnd = lumaEnd2;
	}

	correctVariation = (lumaEnd < 0.0) != isLumaCenterSmaller;

	float finalOffset = 0.0;

	if (correctVariation) {
		finalOffset = pixelOffset;
	}

	float lumaAverage = (1.0 / 12.0) * (2.0 * (lumaDownUp + lumaLeftRight) + lumaLeftCorners + lumaRightCorners);

	float subPixelOffset1 = clamp(abs(lumaAverage - lumaCenter) / lumaRange, 0.0, 1.0);
	float subPixelOffset2 = (-2.0 * subPixelOffset1 + 3.0) * subPixelOffset1 * subPixelOffset1;
	float subPixelOffsetFinal = subPixelOffset2 * subPixelOffset2 * SUBPIXEL_QUALITY;

	finalOffset = max(finalOffset, subPixelOffsetFinal);

	float2 finalTexCoord = input.texCoord;
	if (isHorizontal) {
		finalTexCoord.y += finalOffset * stepLength;
	}
	else {
		finalTexCoord.x += finalOffset * stepLength;
	}

	finalColor = tex.Sample(PSss, finalTexCoord).rgba;

	return finalColor;
}

