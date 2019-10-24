#define HLSL
#include "Common_hlsl_cpp.hlsl"

float4 renderDecal(uint index, float3 vsPosition, float3 wPos, float3 wNorm, float4 payloadColour)
{
    DecalData currDecal = CB_DecalData.data[index];

    // AABB early return test
    float3 minPos = currDecal.position - currDecal.halfSize;
    float3 maxPos = currDecal.position + currDecal.halfSize;
    float4 colourToReturn = 0.f;
    if (wPos.x < minPos.x || wPos.x > maxPos.x ||
        wPos.y < minPos.y || wPos.y > maxPos.y ||
        wPos.z < minPos.z || wPos.z > maxPos.z) {
        return colourToReturn;
    }
    
    // Calculated per pixel to eliminate texture stretching
    float3x3 rotMat = (float3x3)currDecal.rot; //{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
    if (abs(wNorm.z) != 1.f) {
        float3 b = float3(0.f, 0.f, 1.f * sign(wNorm.z));
        float3 v = cross(b, wNorm);
        float angle = acos((wNorm.z * b.z) / (length(b) * length(wNorm)));
        rotMat = mul(Utils::rotateMatrix(angle, v), rotMat);
    }

    float3 pos = currDecal.position;
    float3x3 rot = rotMat;
    float3 size = currDecal.halfSize * 2.f;

    //float3 ddx = ddx_coarse(vsPosition).xyz;
    float3 posNeighborX = vsPosition;

    //float3 ddy = ddy_coarse(vsPosition).xyz;
    float3 posNeighborY = vsPosition;

    // Apply projection, discard if outside its bb
    float3 localPos = wPos - pos;
    localPos = mul(localPos, rot);
    float3 decalUVW = localPos * size;
    decalUVW *= -1;

    if (!(decalUVW.x >= -1.0f && decalUVW.x <= 1.0f &&
        decalUVW.y >= -1.0f && decalUVW.y <= 1.0f &&
        decalUVW.z >= -1.0f && decalUVW.z <= 1.0f))
    {
        return colourToReturn;
    }
    else
    {

        // Get current decal textures
        float2 decalUV = saturate(decalUVW.xy * 0.5f + 0.5f);

        // Calculate gradient
        float3 decalPosNeighborX = posNeighborX - pos;
        decalPosNeighborX = mul(decalPosNeighborX, rot);
        decalPosNeighborX = decalPosNeighborX * size;
        decalPosNeighborX.y *= -1;
        float2 uvDX = saturate(decalPosNeighborX.xy * 0.5f + 0.5f) - decalUV;

        float3 decalPosNeighborY = posNeighborY - pos;
        decalPosNeighborY = mul(decalPosNeighborY, rot);
        decalPosNeighborY = decalPosNeighborY * size;
        decalPosNeighborY.y *= -1;
        float2 uvDY = saturate(decalPosNeighborY.xy * 0.5f + 0.5f) - decalUV;

        // float4 albedoColour = decal_texAlbedo.SampleGrad(ss, decalUV, uvDX, uvDY, 0);
        float4 albedoColour = decal_texAlbedo.SampleLevel(ss, decalUV, 0);
        // float3 decalNormalTS = decal_texNormal.SampleGrad(ss, decalUV, uvDX, uvDY, 0);
        float3 decalNormalTS = decal_texNormal.SampleLevel(ss, decalUV, 0).rgb;
        decalNormalTS = decalNormalTS * 2.0f - 1.0f;
        decalNormalTS.z *= -1.0f;
        float3 decalNormalWS = mul(decalNormalTS, rot);
        // float3 mra = decal_texMetalnessRoughnessAO.SampleGrad(ss, decalUV, uvDX, uvDY, 0);
        float3 mra = decal_texMetalnessRoughnessAO.SampleLevel(ss, decalUV, 0).rgb;
        float metalness = mra.r;
        float roughness = mra.g;
        float ao = mra.b;

        // Removed some black edges
        if (albedoColour.a < 0.4f) {
            return colourToReturn;
        }


        // Blend the decal properties with the material properties
        RayPayload decalPayload;
        decalPayload.color = 0.f;
        decalPayload.recursionDepth = 0;

        // Normal mapping, needs tangents and bitangents!
        // wNorm.y = 1.0f - wNorm.y;
		// wNorm = mul(normalize(wNorm * 2.f - 1.f), tbn);

        float3 norm = lerp(wNorm, decalNormalWS, albedoColour.w);
        shade(wPos, wNorm, payloadColour.rgb, metalness, roughness, ao, decalPayload);
        //colourToReturn = lerp(float4(albedo, 1.0f), decalPayload.color, albedoColour.w);
        colourToReturn = decalPayload.color;
        // colourToReturn = 1.0f;
        // colourToReturn = float4(currDecal.rot[0].rgb, 1.0f);
        //colourToReturn = albedoColour;
    }


    return colourToReturn;
}