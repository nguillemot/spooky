cbuffer LightCBV : register(b0)
{
	float4 LightColor;
	float4 LightPosition;
	float  LightIntensity;
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