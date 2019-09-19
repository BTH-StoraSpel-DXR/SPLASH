#include "Phong.hlsl"

struct VSIn {
	float4 position : POSITION0;
	float2 texCoords : TEXCOORD0;
	float3 normal : NORMAL0;
	float3 tangent : TANGENT0;
	float3 bitangent : BINORMAL0;
};

struct PSIn {
	float4 position : SV_Position;
	float3 normal : NORMAL0;
	float2 texCoords : TEXCOORD0;
	float clip : SV_ClipDistance0;
	float3 toCam : TOCAM;
	//Material material : MAT;
	LightList lights : LIGHTS;
};

cbuffer VSPSSystemCBuffer : register(b0) {
	matrix sys_mWorld;
    matrix sys_mVP;
    Material sys_material;
    //float padding;
    float4 sys_clippingPlane;
    float3 sys_cameraPos;
}

struct PointLightInput {
	float3 color;
	float3 position;
    float attConstant;
    //float attLinear;
    //float attQuadratic;
};
cbuffer VSLights : register(b1) {
	DirectionalLight dirLight;
    PointLightInput pointLights[NUM_POINT_LIGHTS];
}

PSIn VSMain(VSIn input) {
	PSIn output;

	// Copy over the directional light
	output.lights.dirLight = dirLight;
	// Copy over point lights
    for (uint i = 0; i < dirLight.direction.x; i++) {
        output.lights.pointLights[i].attConstant = pointLights[i].attConstant;
        output.lights.pointLights[i].attLinear = 0.1f;
        output.lights.pointLights[i].attQuadratic = 0.02f;
        //output.lights.pointLights[i].attLinear = pointLights[i].attLinear;
        //output.lights.pointLights[i].attQuadratic = pointLights[i].attQuadratic;
        output.lights.pointLights[i].color = pointLights[i].color;
    }

	input.position.w = 1.f;
	output.position = mul(sys_mWorld, input.position);

	// Calculate the distance from the vertex to the clipping plane
	// This needs to be done with world coordinates
    output.clip = dot(output.position, sys_clippingPlane);

	// World space vector pointing from the vertex position to the camera
    output.toCam = sys_cameraPos - output.position.xyz;

    for (uint i = 0; i < NUM_POINT_LIGHTS; i++) {
		// World space vector poiting from the vertex position to the point light
        output.lights.pointLights[i].fragToLight = pointLights[i].position - output.position.xyz;
		// The world space distance from the vertex to the light
        output.lights.pointLights[i].distanceToLight = length(output.lights.pointLights[i].fragToLight);
    }


    output.position = mul(sys_mVP, output.position);

	if (sys_material.hasNormalTexture) {
	    // Convert to tangent space
		float3x3 TBN = {
			mul((float3x3) sys_mWorld, normalize(input.tangent)),
			mul((float3x3) sys_mWorld, normalize(input.bitangent)),
			mul((float3x3) sys_mWorld, normalize(input.normal))
		};
		TBN = transpose(TBN);

		output.toCam = mul(output.toCam, TBN);
		output.lights.dirLight.direction = mul(output.lights.dirLight.direction, TBN);
        for (int i = 0; i < NUM_POINT_LIGHTS; i++)
            output.lights.pointLights[i].fragToLight = mul(output.lights.pointLights[i].fragToLight, TBN);
    }

	output.normal = mul((float3x3) sys_mWorld, input.normal);
	output.normal = normalize(output.normal);

	output.texCoords = input.texCoords;

	return output;

}

Texture2D sys_texDiffuse : register(t0);
Texture2D sys_texNormal : register(t1);
Texture2D sys_texSpecular : register(t2);
SamplerState PSss : register(s0);

float4 PSMain(PSIn input) : SV_Target0 {

	PhongInput phongInput;
	phongInput.mat = sys_material;
	phongInput.fragToCam = input.toCam;
	phongInput.lights = input.lights;

	phongInput.diffuseColor = sys_material.modelColor;
	if (sys_material.hasDiffuseTexture)
		phongInput.diffuseColor *= sys_texDiffuse.Sample(PSss, input.texCoords);

	phongInput.normal = input.normal;
	if (sys_material.hasNormalTexture)
		phongInput.normal = sys_texNormal.Sample(PSss, input.texCoords).rgb * 2.f - 1.f;

	phongInput.specMap = float3(1.f, 1.f, 1.f);
	if (sys_material.hasSpecularTexture)
		phongInput.specMap = sys_texSpecular.Sample(PSss, input.texCoords).rgb;


    //return sys_texDiffuse.Sample(PSss, input.texCoords);
	// return float4(phongInput.normal * 0.5f + 0.5, 1.f);
    return phongShade(phongInput);
    //return float4(phongInput.lights.dirLight.direction, 1.f);
    //return float4(phongInput.diffuseColor.rgb, 1.f);
    //return float4(0.f, 1.f, 0.f, 1.f);

}

