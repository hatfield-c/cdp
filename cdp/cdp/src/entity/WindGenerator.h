#pragma once

#include "../engine/Transform.h"

struct WindGenerator {
	long seed = 555586;

	Vector3 RandomWind() {
		Vector3 wind_direction{ -1, 0, 0 };

		float wind_strength = 500000 * this->FloatNoise();

		return wind_direction * wind_strength;
	}

	Vector3 VectorNoise() {
		Vector3 noise{
			this->FloatNoise(),
			this->FloatNoise(),
			this->FloatNoise()
		};

		return noise;
	}

	float FloatNoise() {
		long noisy_bits = this->NextSample(this->seed);
		float noise = (float)noisy_bits / 32768.0f;
		noise = (2 * noise) - 1;
		this->seed = noisy_bits;
		
		return noise;
	}

	long NextSample(long current) {
		long next = current * 1103515245 + 12345;
		next = (unsigned)(next / 65536) % 32768;

		return next;
	}
};