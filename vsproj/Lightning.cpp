#include "Lightning.h"

#include <random>

Lightning::Lightning() :
	flashing(false),
	intensity(0.f),
	delay(0.f)
{
}


Lightning::~Lightning()
{
}

bool Lightning::IsFlashing()
{
	return flashing;
}

void Lightning::GenerateFlash()
{
	std::default_random_engine generator;
	std::normal_distribution<float> delayDist(6.f, 4.f);

	delay = delayDist(generator);

	flashing = true;
}

void Lightning::doFlash(int deltaTime_ms)
{
	if (delay > 0.f) {
		delay -= deltaTime_ms / 1000.f;
		if (delay <= 0.f) {
			delay = 0.f;
			intensity = .5f;
		}
	}
	else {
		intensity -= deltaTime_ms / 1000.f / flashDuration;
		if (intensity <= 0.f) {
			intensity = 0.f;
			flashing = false;
		}
	}
}

float Lightning::GetIntensity() {
	return intensity;
}
