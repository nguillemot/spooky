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
};

struct PS_OUTPUT
{
    float4 Color : SV_Target;
};

/*
float4 calcPhongLighting(Material M, float4 LColor, float3 N, float3 L, float3 V, float3 R)
{
	float4 Ia = M.Ka * 0.1f;
	float4 Id = M.Kd * saturate(dot(N, L));
	float4 Is = M.Ks * pow(saturate(dot(R, V)), M.A);

	return Ia + (Id + Is) * LColor;
}
*/


PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
	float3 NormalizedWorldNormal = normalize(input.WorldNormal);
	float3 NormalizedWorldPositionToLight = normalize(LightPosition.xyz -input.WorldPosition.xyz);
	float GeometricTerm = max(0, dot(NormalizedWorldNormal, NormalizedWorldPositionToLight));
	output.Color = mul(mul(LightColor, GeometricTerm), LightIntensity);
    //output.Color = float4(input.Position.www / 100.0f, 1.0);
    return output;
}