Texture2D FogTexture : register(t0);
SamplerState FogSampler : register(s0);

struct PS_INPUT
{
    float4 Position : SV_Position;
    float Intensity : INTENSITY;
    float2 TexCoord : TEXCOORD;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target;
};

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    float4 textureColor = FogTexture.Sample(FogSampler, input.TexCoord);
    output.Color = float4(textureColor.rgb, textureColor.a * input.Intensity);
    return output;
}