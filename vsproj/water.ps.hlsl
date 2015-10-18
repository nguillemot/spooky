Texture2D WaterDepthTexture : register(t0);
SamplerState WaterDepthSampler : register(s0);

struct PS_INPUT
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target;
};

static const float4 kDepthColor = float4(0, 0, 0, 1);
static const float4 kShallowColor = float4(0.0, 0.05, 0.06, 1);

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    float waterDepth = WaterDepthTexture.Sample(WaterDepthSampler, input.TexCoord).r;
    float4 waterColor = lerp(kDepthColor, kShallowColor, waterDepth);
    output.Color = float4(waterColor.xyz, pow((1.0 - waterDepth),2));
    return output;
}