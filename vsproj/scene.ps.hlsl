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

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
	float3 NormalizedWorldNormal = normalize(input.WorldNormal);
	float3 NormalizedWorldPositionToLight = normalize(LightPosition.xyz -input.WorldPosition.xyz);
	float3 NormalizedWorldPositionToCamera = normalize(input.WorldPosition.xyz - input.WorldCameraPosition.xyz);
	float3 LightReflectionAroundNormal = normalize(reflect(NormalizedWorldPositionToLight, NormalizedWorldNormal));

	float3 Ia = float3(0.f, 0.f, 0.f);
	float3 Id = DiffuseColor * max(0, dot(NormalizedWorldNormal, NormalizedWorldPositionToLight)) * LightIntensity;
	float LightDistanceSquared = length(pow(input.WorldPosition.xyz - LightPosition.xyz, 2));
	Id *= (LightDistanceSquared / dot(LightPosition.xyz - input.WorldPosition.xyz, LightPosition.xyz - input.WorldPosition.xyz));
	float3 Is = max(0, dot(LightReflectionAroundNormal, NormalizedWorldPositionToCamera));
	output.Color = float4(Ia + Id + Is, 0.f) * LightColor;
	//float GeometricTerm = max(0, dot(NormalizedWorldNormal, NormalizedWorldPositionToLight));
	//output.Color = mul(mul(LightColor, GeometricTerm), LightIntensity);
    //output.Color = float4(input.Position.www / 100.0f, 1.0);
    return output;
}