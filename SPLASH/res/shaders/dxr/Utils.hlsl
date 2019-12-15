namespace Utils {

#ifdef RAYTRACING
    // Retrieve hit world position.
    float3 HitWorldPosition() {
        return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
    }

    RayDesc getRayDesc(float3 direction, float3 origin = HitWorldPosition(), float tmax = 100000) {
        RayDesc ray;
        ray.Origin = origin;
        ray.Direction = direction;
        ray.TMin = 0.0001;
        ray.TMax = tmax;
        return ray;
    }

    bool rayHitAnything(float3 origin, float3 normal, float3 direction, float tmax = 100000) {
        RayDesc rayDesc;
        rayDesc.Origin = origin + normal * 0.001f;
        rayDesc.Direction = direction;
        rayDesc.TMin = 0.001;
        rayDesc.TMax = tmax;

        ShadowRayPayload shadowPayload;
		shadowPayload.isHit = true; // Assume hit, miss shader will set to false
		TraceRay(gRtScene, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_CULL_BACK_FACING_TRIANGLES, INSTANCE_MASK_CAST_SHADOWS, 1 /*NULL hit group*/, 0, 1 /*Shadow miss shader*/, rayDesc, shadowPayload);

		return shadowPayload.isHit;
    }

    RayPayload makeEmptyPayload() {
        RayPayload payload;
        payload.recursionDepth = 0;
        payload.closestTvalue = 0;
#ifdef ALLOW_SOFT_SHADOWS
        for (uint i = 0; i < NUM_SHADOW_TEXTURES; i++) {
            payload.shadowTwo[i] = 0.f;
        }
#endif
        payload.albedoOne = 0.f;
        payload.albedoTwo = 0.f;
        payload.normalOne = 0.f;
        payload.normalTwo = 0.f;
        payload.metalnessRoughnessAOOne = 0.f;
        payload.metalnessRoughnessAOTwo = 0.f;
        payload.worldPositionOne = 0.f;
        payload.worldPositionTwo = 0.f;
        return payload;
    }
#endif

    // Barycentric interpolation
    float2 barrypolation(float3 barry, float2 in1, float2 in2, float2 in3) {
        return barry.x * in1 + barry.y * in2 + barry.z * in3;
    }
    float3 barrypolation(float3 barry, float3 in1, float3 in2, float3 in3) {
        return barry.x * in1 + barry.y * in2 + barry.z * in3;
    }

    // Generates a seed for a random number generator from 2 inputs plus a backoff
    uint initRand(uint val0, uint val1, uint backoff = 16) {
        uint v0 = val0, v1 = val1, s0 = 0;

        [unroll]
        for (uint n = 0; n < backoff; n++)
        {
            s0 += 0x9e3779b9;
            v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
            v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
        }
        return v0;
    }

    // Takes our seed, updates it, and returns a pseudorandom float in [0..1]
    float nextRand(inout uint s) {
        s = (1664525u * s + 1013904223u);
        return float(s & 0x00FFFFFF) / float(0x01000000);
    }

    // Utility function to get a vector perpendicular to an input vector 
    //    (from "Efficient Construction of Perpendicular Vectors Without Branching")
    float3 getPerpendicularVector(float3 u) {
        float3 a = abs(u);
        uint xm = ((a.x - a.y)<0 && (a.x - a.z)<0) ? 1 : 0;
        uint ym = (a.y - a.z)<0 ? (1 ^ xm) : 0;
        uint zm = 1 ^ (xm | ym);
        return cross(u, float3(xm, ym, zm));
    }

    // Get a cosine-weighted random vector centered around a specified normal direction.
    float3 getCosHemisphereSample(inout uint randSeed, float3 hitNorm) {
        // Get 2 random numbers to select our sample with
        float2 randVal = float2(nextRand(randSeed), nextRand(randSeed));

        // Cosine weighted hemisphere sample from RNG
        float3 bitangent = getPerpendicularVector(hitNorm);
        float3 tangent = cross(bitangent, hitNorm);
        float r = sqrt(randVal.x);
        float phi = 2.0f * 3.14159265f * randVal.y;

        // Get our cosine-weighted hemisphere lobe sample direction
        return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * sqrt(1 - randVal.x);
    }

    float3x3 rotateMatrix(float angle, float3 axis) {
        float c, s;
        sincos(angle, s, c);

        float t = 1 - c;
        float x = axis.x;
        float y = axis.y;
        float z = axis.z;

        return float3x3(
            t * x * x + c, t * x * y - s * z, t * x * z + s * y,
            t * x * y + s * z, t * y * y + c, t * y * z - s * x,
            t * x * z - s * y, t * y * z + s * x, t * z * z + c
        );
    }

    int to1D(int3 ind, int xMax, int yMax) {
        return ind.x + xMax * (ind.y + yMax * ind.z);
    }

    int3 to3D(int ind, int xMax, int yMax) {
        int3 ind3d;
        ind3d.z = ind / (xMax * yMax);
        ind -= (ind3d.z * xMax * yMax);
        ind3d.y = ind / xMax;
        ind3d.x = ind % xMax;
        return ind3d;
    }

    uint unpackQuarterFloat(uint input, uint index) {
        uint output = (input >> ((3-index) * 8)) & 255;
        // output = max(1, output);
        return output;
    }

    // Rotation with angle (in radians) and axis
    float3x3 angleAxis3x3(float angle, float3 axis) {
        float c, s;
        sincos(angle, s, c);

        float t = 1 - c;
        float x = axis.x;
        float y = axis.y;
        float z = axis.z;

        return float3x3(
            t * x * x + c,      t * x * y - s * z,  t * x * z + s * y,
            t * x * y + s * z,  t * y * y + c,      t * y * z - s * x,
            t * x * z - s * y,  t * y * z + s * x,  t * z * z + c
        );
    }

    // Returns a random direction vector inside a cone
    // Angle defined in radians
    // Example: direction=(0,1,0) and angle=pi returns ([-1,1],[0,1],[-1,1])
    float3 getConeSample(inout uint randSeed, float3 direction, float coneAngle) {
        float cosAngle = cos(coneAngle);

        // Generate points on the spherical cap around the north pole [1].
        // [1] See https://math.stackexchange.com/a/205589/81266
        float z = nextRand(randSeed) * (1.0f - cosAngle) + cosAngle;
        float phi = nextRand(randSeed) * 2.0f * PI;
        
        float x = sqrt(1.0f - z * z) * cos(phi);
        float y = sqrt(1.0f - z * z) * sin(phi);
        float3 north = float3(0.f, 0.f, 1.f);

        // Find the rotation axis `u` and rotation angle `rot` [1]
        float3 axis = normalize(cross(north, normalize(direction)));
        float angle = acos(dot(normalize(direction), north));

        // Convert rotation axis and angle to 3x3 rotation matrix [2]
        float3x3 R = angleAxis3x3(angle, axis);

        return mul(R, float3(x, y, z));
    }

}