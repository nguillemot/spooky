cbuffer LightCBV : register(b0)
{
    float4 LightColor;
    float4 LightPosition;
    float4 AmbientLightColor;
    float  LightIntensity;
	float  LightningIntensity;
};

cbuffer MaterialCBV : register(b1)
{
    float3 AmbientColor;
    float3 DiffuseColor;
    float3 SpecularColor;
    float Ns;
};

struct PS_INPUT
{
    float4 Position		 : SV_Position;
    float3 WorldNormal   : WORLDNORMAL;
    float4 WorldPosition : WORLDPOSITION;
    float4 WorldCameraPosition : WORLDCAMERAPOS;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target;
};

/*PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    float3 NormalizedWorldNormal = normalize(input.WorldNormal);
    float3 NormalizedWorldPositionToLight = normalize(LightPosition.xyz -input.WorldPosition.xyz);
    float3 NormalizedWorldPositionToCamera = normalize(input.WorldPosition.xyz - input.WorldCameraPosition.xyz);
    float3 LightReflectionAroundNormal = normalize(reflect(NormalizedWorldPositionToLight, NormalizedWorldNormal));

    float3 AmbientIntensity = dot(normalize(AmbientLightDirection), float4(NormalizedWorldNormal, 1.f));

    float3 Ia = AmbientLightColor * AmbientIntensity;
    float3 Id = DiffuseColor * max(0, dot(NormalizedWorldNormal, NormalizedWorldPositionToLight));
    float LightDistanceSquared = length(pow(input.WorldPosition.xyz - LightPosition.xyz, 2));
    Id *= (LightDistanceSquared / dot(LightPosition.xyz - input.WorldPosition.xyz, LightPosition.xyz - input.WorldPosition.xyz));
    float3 Is = max(0, dot(LightReflectionAroundNormal, NormalizedWorldPositionToCamera));
    output.Color = float4((Id + Is), 0.f) * LightColor * LightIntensity;
    //float GeometricTerm = max(0, dot(NormalizedWorldNormal, NormalizedWorldPositionToLight));
    //output.Color = mul(mul(LightColor, GeometricTerm), LightIntensity);
    //output.Color = float4(input.Position.www / 100.0f, 1.0);
    return output;
}*/

PS_OUTPUT main(PS_INPUT input) {
    PS_OUTPUT output;

    float4 NormalizedWorldPositionToCamera = normalize(input.WorldCameraPosition - input.WorldPosition);

    float DistanceFromCamera = length(input.WorldCameraPosition - input.WorldPosition);
    float3 aColor = (AmbientLightColor.xyz + AmbientColor) * dot(input.WorldNormal.xyz, NormalizedWorldPositionToCamera.xyz) / pow(DistanceFromCamera, 2) + (float3(1.f, 1.f, 1.f) * LightningIntensity);

    float4 LightDirectionToPosition = normalize(LightPosition - input.WorldPosition);
    float LightToWorldPositionDistanceSquared = pow(length(input.WorldPosition - LightPosition), 1);
    float DiffuseIntensity = max(0, dot(LightDirectionToPosition, normalize(float4(input.WorldNormal, 0.f))));
    float3 dColor = ((DiffuseColor / 2) + LightColor.xyz) * DiffuseIntensity * LightIntensity;

    float4 NormalizedWorldPositionToLight = normalize(input.WorldPosition - LightPosition);
    float4 NormalizedWorldNormal = float4(normalize(input.WorldNormal), 0.f);

    float4 LightReflectionAroundNormal = normalize(reflect(NormalizedWorldPositionToLight, NormalizedWorldNormal));
    float SpecularIntensity = pow(max(0, dot(NormalizedWorldPositionToCamera, LightReflectionAroundNormal)), 2);

    float3 sColor = float3(1.f, 1.f, 1.f) / 4 * SpecularIntensity;

    output.Color = float4(aColor + (dColor + sColor), 0.f);
    //output.Color = AmbientLightColor;
    //output.Color = aColor + float4(DiffuseColor * DiffuseIntensity, 0.f);
    return output;
}