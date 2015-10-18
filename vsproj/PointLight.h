#pragma once

#include <DirectXMath.h>
#include "dxutil.h"
#include <memory>

__declspec(align(16))
struct LightData
{
	DirectX::XMFLOAT4 LightColor;
	DirectX::XMFLOAT4 LightPosition;
	DirectX::XMFLOAT4 AmbientLightColor;
	float LightIntensity;
};

class PointLight
{
public:
	PointLight();
	PointLight(const PointLight & copy_from);
	~PointLight();

	void SetColor(float r, float g, float b, float a);
	void SetAmbientColor(float r, float g, float b, float a);
	void SetPosition(float x, float y, float z);
	void SetIntensity(float i);

	DirectX::XMFLOAT4 GetColor();
	DirectX::XMFLOAT4 GetPosition();
	DirectX::XMFLOAT4 GetAmbientColor();
	float GetIntensity();

	LightData* data();

private:
	LightData pLight;
};

