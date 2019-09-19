//
// Based of https://pages.mtu.edu/~shene/PUBLICATIONS/2004/CCSC-MW-2004-depth_of_field.pdf
// Reference http://fileadmin.cs.lth.se/cs/education/edan35/lectures/12dof.pdf
//

Texture2D<float4> colorInput : register(t0);
Texture2D<float4> depthInput : register(t1);
RWTexture2D<float4> output : register(u0);

RWStructuredBuffer<int4> sharedOutput;

cbuffer cb : register(b0) {
	float zNear;
	float zFar;
}

void uniformDistribution(float coc, uint2 pixCoord) {
	static const int maxCocRadius = 20;
	// Radius of the circle of confusion
	int r = max(coc / 2.f, 1.f);
	// CoC area
	float a = 3.1415926f * r * r;
	float4 intensity = colorInput[pixCoord];
	intensity /= a;
	for (int x = -maxCocRadius; x < maxCocRadius; x++) {
		for (int y = -maxCocRadius; y < maxCocRadius; y++) {
			if (x*x+y*y < r * r) {
				//sharedOutput[(pixCoord.y + y) * 1280 + (pixCoord.x + x)] += intensity;
				//InterlockedAdd(sharedOutput[(pixCoord.y + y) * 1280 + (pixCoord.x + x)].r, int(intensity.r * 1000));
				//InterlockedAdd(sharedOutput[(pixCoord.y + y) * 1280 + (pixCoord.x + x)].g, int(intensity.g * 1000));
				//InterlockedAdd(sharedOutput[(pixCoord.y + y) * 1280 + (pixCoord.x + x)].b, int(intensity.b * 1000));
				//InterlockedAdd(sharedOutput[(pixCoord.y + y) * 1280 + (pixCoord.x + x)].a, int(intensity.a * 1000));
				output[pixCoord + int2(x, y)] += intensity;
			}
		}
	}
	//sharedOutput[(pixCoord.y) * 1280 + (pixCoord.x)] += float4(1.f, 0.f, 0.f, 1.f);
	//output[pixCoord + int2(-10, 0)] += intensity;
}

#define NUM_THREADS 1024
[numthreads(NUM_THREADS, 1, 1)]
void CSMain(uint3 DispatchThreadID : SV_DispatchThreadID) {

	uint2 pixCoords = uint2(DispatchThreadID.x + NUM_THREADS * DispatchThreadID.z, DispatchThreadID.y);

	// Aperture
	static const float a = 2.0f;
	// Screen position
	static const float s = 10.25f;
	// Focal length
	static const float f = 1.0f;
	// Distance to pixel (read from depth buffer) 
	float d = zNear*zFar / (zFar - depthInput[pixCoords].r*(zFar-zNear));

	float coc = abs(a*(s*(1.f / f - 1.f / d) - 1));

	GroupMemoryBarrierWithGroupSync();
	uniformDistribution(coc, pixCoords);

	//output[pixCoords] = sharedOutput[(pixCoords.y) * 1280 + (pixCoords.x)] / 1000.f;


	//output[pixCoords] = color;
	//output[pixCoords] = input[pixCoords] + float4(0.3f, 0.f, 0.f, 0.f);
}