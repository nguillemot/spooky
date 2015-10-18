#include "common.hlsl"

cbuffer CameraCBV : register(b0)
{
    CameraData Camera;
};

struct GS_INPUT
{
    float4 Position : SV_Position;
    float Intensity : INTENSITY;
};

struct GS_OUTPUT
{
    float4 Position : SV_Position;
    float Intensity : INTENSITY;
    float2 TexCoord : TEXCOORD;
};

static const float kParticleRadius = 1.0;

[maxvertexcount(4)]
void main(point GS_INPUT input[1], inout TriangleStream<GS_OUTPUT> triStream)
{
    float4 viewPosition = mul(input[0].Position, Camera.WorldView);
    float4 across = float4(kParticleRadius, 0.0, 0.0, 0.0);
    float4 up = float4(0.0, kParticleRadius, 0.0, 0.0);

    GS_OUTPUT output;

    output.Intensity = input[0].Intensity;

    output.Position = mul(viewPosition + across - up, Camera.ViewProjection);
    output.TexCoord = float2(1, 1);
    triStream.Append(output);

    output.Position = mul(viewPosition - across - up, Camera.ViewProjection);
    output.TexCoord = float2(0, 1);
    triStream.Append(output);

    output.Position = mul(viewPosition + across + up, Camera.ViewProjection);
    output.TexCoord = float2(1, 0);
    triStream.Append(output);

    output.Position = mul(viewPosition - across + up, Camera.ViewProjection);
    output.TexCoord = float2(0, 0);
    triStream.Append(output);
}