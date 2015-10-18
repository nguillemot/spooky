#include "common.hlsl"

cbuffer CameraCBV : register(b0)
{
    CameraData Camera;
};

struct VS_INPUT
{
    uint VertexID : SV_VertexID;
};

struct VS_OUTPUT
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD;
};

static const float kWaterRadius = 20.0;
static const float kWaterHeight = -3.0;

static const float3 kWorldPositions[] = {
    float3(+kWaterRadius, kWaterHeight, +kWaterRadius),
    float3(+kWaterRadius, kWaterHeight, -kWaterRadius),
    float3(-kWaterRadius, kWaterHeight, -kWaterRadius),
    float3(-kWaterRadius, kWaterHeight, -kWaterRadius),
    float3(-kWaterRadius, kWaterHeight, +kWaterRadius),
    float3(+kWaterRadius, kWaterHeight, +kWaterRadius),
};

static const float2 kTexCoords[] = {
    float2(1, 1),
    float2(1, 0),
    float2(0, 0),
    float2(0, 0),
    float2(0, 1),
    float2(1, 1)
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.Position = mul(float4(kWorldPositions[input.VertexID], 1.0), Camera.WorldViewProjection);
    output.TexCoord = kTexCoords[input.VertexID];
    return output;
}