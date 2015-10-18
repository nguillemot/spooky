struct CameraData
{
    float4x4 WorldViewProjection;
    float4x4 WorldView;
    float4x4 ViewProjection;
    float4 EyePosition;
};

struct TimeData
{
    float TimeSinceStart_sec;
};