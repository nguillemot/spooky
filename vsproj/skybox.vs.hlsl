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
    float4 CubeMapDirection : CUBEMAPDIRECTION;
};

/*
    4 ---- 5
   /|     /|
 /  |   /  |
0 ---- 1   |
|   6 -|-- 7
|  /   |  /
| /    | /
2 ---- 3

     +y
     ^    +z
     |   ^
     |  /
     | / 
   center -----> +x
*/
static const float3 kCorners[8] = {
    float3(-1, +1, -1),
    float3(+1, +1, -1),
    float3(-1, -1, -1),
    float3(+1, -1, -1),
    float3(-1, +1, +1),
    float3(+1, +1, +1),
    float3(-1, -1, +1),
    float3(+1, -1, +1)
};

static const uint kCubeIndices[36] = {
    6, 4, 5,
    6, 5, 7,
    2, 6, 7,
    2, 7, 3,
    2, 0, 4,
    2, 4, 6,
    7, 5, 1,
    7, 1, 3,
    4, 0, 1,
    4, 1, 5,
    3, 1, 0,
    3, 0, 2
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    float4 worldCornerDirection = float4(kCorners[kCubeIndices[input.VertexID]], 0.0);
    float4 worldCornerPosition = float4(Camera.EyePosition.xyz + worldCornerDirection.xyz, 1.0);
    output.Position = mul(worldCornerPosition, Camera.WorldView).xyzz;
    output.CubeMapDirection = worldCornerDirection;
    return output;
}