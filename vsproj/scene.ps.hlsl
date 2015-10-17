struct PS_INPUT
{
    float4 Position : SV_Position;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target;
};

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    output.Color = float4(input.Position.www / 100.0f, 1.0);
    return output;
}