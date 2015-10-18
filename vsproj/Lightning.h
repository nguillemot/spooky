#pragma once
class Lightning
{
	const float flashDuration = 0.5f;
public:
	Lightning();
	~Lightning();

	bool IsFlashing();
	void GenerateFlash();

	void doFlash(int deltaTime_ms);
	float GetIntensity();

private:
	bool flashing;
	float intensity;
	float delay;
};

