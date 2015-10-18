cbuffer LightCBV : register(b0)
{
	float4 LightColor;
	float4 LightPosition;
	float  LightIntensity;
};

cbuffer MaterialCBV : register(b1)
{
	float3 AmbientColor;
	float3 DiffuseColor;
	float3 SpecularColor;
	float Ns;
};

struct PS_INPUT
{
    float4 Position		 : SV_Position;
	float3 WorldNormal   : WORLDNORMAL;
	float4 WorldPosition : WORLDPOSITION;
	float4 WorldCameraPosition : WORLDCAMERAPOS;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target;
};


float4 calcPhongLighting(float3 N, float3 L, float3 V, float3 R)
{
	float3 Ia = AmbientColor * 0;
	float3 Id = DiffuseColor * saturate(dot(N, L));
	float3 Is = SpecularColor * saturate(dot(R, V)); // 0.1f is an arbitrary ambient light intensity because we don't have ambient light

	return float4(Ia + (Id + Is), 1) * LightColor * LightIntensity;
}


PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
	float3 NormalizedWorldNormal = normalize(input.WorldNormal);
	float3 NormalizedWorldPositionToLight = normalize(LightPosition.xyz -input.WorldPosition.xyz);
	float3 NormalizedWorldPositionToCamera = normalize(input.WorldCameraPosition.xyz - input.WorldPosition.xyz);
	float3 LightReflectionAroundNormal = normalize(reflect(NormalizedWorldPositionToLight, NormalizedWorldNormal));
	output.Color = calcPhongLighting(NormalizedWorldNormal, NormalizedWorldPositionToLight, NormalizedWorldPositionToCamera, LightReflectionAroundNormal);
	//float GeometricTerm = max(0, dot(NormalizedWorldNormal, NormalizedWorldPositionToLight));
	//output.Color = mul(mul(LightColor, GeometricTerm), LightIntensity);
    //output.Color = float4(input.Position.www / 100.0f, 1.0);
    return output;
}