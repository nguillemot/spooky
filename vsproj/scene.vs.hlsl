#include "common.hlsl"

cbuffer CameraCBV : register(b0)
{
    CameraData Camera;
};

struct VS_INPUT
{
    float4 Position : POSITION;
    float3 Normal   : NORMAL;
    float4x4 ModelWorld : MODELWORLD;
};

struct VS_OUTPUT
{
    float4 Position : SV_Position;
    float3 WorldNormal   : WORLDNORMAL;
    float4 WorldPosition : WORLDPOSITION;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.Position = mul(mul(input.Position, input.ModelWorld), Camera.WorldViewProjection);
    output.WorldNormal = mul(float4 (input.Normal, 0), input.ModelWorld).xyz;
    output.WorldPosition = mul(input.Position, input.ModelWorld);
    return output;
}