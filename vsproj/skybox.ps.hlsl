cbuffer LightCBV : register(b0)
{
	float4 LightColor;
	float4 LightPosition;
	float4 AmbientLightColor;
	float  LightIntensity;
	float  LightningIntensity;
};

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
	if ((output.Color.x + output.Color.y + output.Color.z / 3.f) > 0.1f) {
		output.Color += float4(1.f, 1.f, 1.f, 1.f) * LightningIntensity;
	}
    return output;
}