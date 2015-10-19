#include "common.hlsl"

Texture2D WaterDepthTexture : register(t0);
SamplerState WaterDepthSampler : register(s0);

cbuffer TimeCBV : register(b0)
{
    TimeData Time;
}

cbuffer LightningCBV : register(b1)
{
	float4 LightColor;
	float4 LightPosition;
	float4 AmbientLightColor;
	float  LightIntensity;
	float  LightningIntensity;
}

struct PS_INPUT
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target;
};

static const float3 kDepthColor = float3(0, 0, 0);
static const float3 kShallowColor = float3(0.0, 0.05, 0.06);

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    float4 sampledDepthTexture = WaterDepthTexture.Sample(WaterDepthSampler, input.TexCoord);
    float landAffinity = 1.0 - sampledDepthTexture.a;
    float waterDepth =  0.5 - sampledDepthTexture.r * 0.3;
    waterDepth += 0.1 * sin(Time.TimeSinceStart_sec);
    waterDepth = frac(waterDepth * 6);

    if (landAffinity < 0.7)
    {
        float3 waterColor = lerp(kDepthColor, kShallowColor, waterDepth);
        waterColor += float3(0.0, 0.01, 0.01) * sin(Time.TimeSinceStart_sec) + (float3(1.f, 1.f, 1.f) * LightningIntensity);
        output.Color = float4(waterColor, pow((1.0 - waterDepth), 2));
    }
    else
    {
        output.Color = float4(0,0,0,0);
    }

    return output;
}