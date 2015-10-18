TextureCube SkyboxTexture : register(t0);
SamplerState SkyboxSampler : register(s0);

struct PS_INPUT
{
    float4 Position : SV_Position;
    float3 CubeMapDirection : CUBEMAPDIRECTION;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target;
};

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    output.Color = SkyboxTexture.Sample(SkyboxSampler, input.CubeMapDirection.xyz);
    return output;
}