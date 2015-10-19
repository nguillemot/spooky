#include "PointLight.h"

#include <iostream>

PointLight::PointLight()
{
}


PointLight::~PointLight()
{
}

PointLight::PointLight(const PointLight& copy_from)
{
	pLight.LightColor = copy_from.pLight.LightColor;
	pLight.LightPosition = copy_from.pLight.LightPosition;
	pLight.AmbientLightColor = copy_from.pLight.AmbientLightColor;
	pLight.LightIntensity = copy_from.pLight.LightIntensity;
}

DirectX::XMFLOAT4 PointLight::GetColor()
{
	return pLight.LightColor;
}

DirectX::XMFLOAT4 PointLight::GetPosition()
{
	return pLight.LightPosition;
}

DirectX::XMFLOAT4 PointLight::GetAmbientColor()
{
	return pLight.AmbientLightColor;
}

float PointLight::GetIntensity()
{
	return pLight.LightIntensity;
}


void PointLight::SetColor(float r, float g, float b, float a) 
{
	DirectX::XMFLOAT4 color(r, g, b, a);
	pLight.LightPosition = color;
}

void PointLight::SetPosition(float x, float y, float z) 
{
	DirectX::XMFLOAT4 position(x, y, z, 1.f);
	pLight.LightPosition = position;
}

void PointLight::SetAmbientColor(float r, float g, float b, float a)
{
	DirectX::XMFLOAT4 color(r, g, b, a);
	pLight.AmbientLightColor = color;
}

void PointLight::SetIntensity(float i)
{
	pLight.LightIntensity = i;
}

LightData* PointLight::data() 
{
	return (LightData*) &pLight;
}