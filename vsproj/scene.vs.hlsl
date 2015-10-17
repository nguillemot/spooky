#include "common.hlsl"

cbuffer CameraCBV : register(b0)
{
    CameraData Camera;
};

struct VS_INPUT
{
    float4 Position : POSITION;
    float4x4 ModelWorld : MODELWORLD;
};

struct VS_OUTPUT
{
    float4 Position : SV_Position;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.Position = mul(mul(input.Position, input.ModelWorld), Camera.WorldViewProjection);
    return output;
}