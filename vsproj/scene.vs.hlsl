cbuffer CameraCBV : register(b0)
{
    float4x4 WorldViewProjection;
};

struct VS_INPUT
{
    float4 Position : SV_Position;
    float4x4 ModelWorld : MODELWORLD;
};

struct VS_OUTPUT
{
    float4 Position : SV_Position;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.Position = mul(mul(input.Position, input.ModelWorld), WorldViewProjection);
    return output;
}