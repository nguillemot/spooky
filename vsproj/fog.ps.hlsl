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
    
    // turn the 1->0 intensity into an out-in-out pattern
    // 1 ---> 0                # original intensity input
    // 1 ---> 0 ---> -1        # extend line to negative
    // 1 ---> 0 ----> 1        # absolute value
    // 0 ---> 1 ---> 0         # 1 - x
    float intensity = input.Intensity;
    intensity = intensity * 2 - 1;
    intensity = abs(intensity);
    intensity = 1 - intensity;

    float4 textureColor = FogTexture.Sample(FogSampler, input.TexCoord);
    output.Color = float4(textureColor.rgb, textureColor.a * intensity);
    return output;
}