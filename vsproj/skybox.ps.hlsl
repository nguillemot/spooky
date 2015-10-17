TextureCube SkyboxTexture : register(t0);
SamplerState SkyboxSampler : register(s0);

struct PS_INPUT
{
    float4 Position : SV_Position;
    float4 CubeMapDirection : CUBEMAPDIRECTION;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target;
};

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    // output.Color = float4(normalize(input.CubeMapDirection.xyz), 1.0);
    output.Color = SkyboxTexture.Sample(SkyboxSampler, input.CubeMapDirection.xyz, 0);
    return output;
}