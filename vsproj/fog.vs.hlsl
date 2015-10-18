struct VS_INPUT
{
    float4 Position : POSITION;
    float Intensity : INTENSITY;
};

struct VS_OUTPUT
{
    float4 Position : SV_Position;
    float Intensity : INTENSITY;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.Position = input.Position;
    output.Intensity = input.Intensity;
    return output;
}