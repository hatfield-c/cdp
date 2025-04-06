#pragma once

#include "../engine/RigidBody.h"
#include "ai/Wallrider.h"
#include "ai/PolyField.h"

struct DroneAlpha {
	Rigidbody rigidbody{};
	Wallrider wallrider{};
	PolyField poly_field{};

	float forward_max = 1;
	float climb_max = 1;
	float yaw_max = 0.5;

	Vector3 drift_direction{ 0, -1, 0 };
	float drift_size = 0.5f;
	float drift_delta = 0.4f;

	long seed = 555586;

	void Init() {
		this->rigidbody.Init();
		this->wallrider.Init();
		this->poly_field.Init(256, 256, 256, 2);
	}

	void Update(float* depth_phash, Vector3* camera_cloud) {
		this->wallrider.Update(depth_phash, camera_cloud, this->rigidbody.velocity);
		//this->poly_field.Update(depth_phash);
	}

	void Act() {
		Vector3 command = this->wallrider.GetCommand(this->rigidbody.velocity);
		this->FollowCommand(command);
	}

	void FollowCommand(Vector3 command) {
		float yaw_target = -command.x * this->yaw_max;
		this->rigidbody.angular_velocity.y = (0.4f * yaw_target) + (0.6f * this->rigidbody.angular_velocity.y);

		Vector3 body_forward = this->rigidbody.Forward();
		body_forward.y = 0;
		Vector3 velocity_target = body_forward * this->forward_max * command.z;
		
		velocity_target.y += command.y * this->climb_max;
		this->rigidbody.velocity = (velocity_target * 0.4f) + (this->rigidbody.velocity * 0.6f);

		//Vector3 xz_velocity = body_forward * command.z * 2;

		//Vector3 force = Vector3{ xz_velocity.x, command.y * 5.0f, xz_velocity.z };
		//this->rigidbody.AddForce(force);
	}

	void Drift() {
		long noisy_bits = this->NextSample(this->seed);
		float noise_x = (float)noisy_bits / 32768.0f;
		noise_x = (2 * noise_x) - 1;

		noisy_bits = this->NextSample(noisy_bits);
		float noise_y = (float)noisy_bits / 32768.0f;
		noise_y = (2 * noise_x) - 1;

		noisy_bits = this->NextSample(noisy_bits);
		float noise_z = (float)noisy_bits / 32768.0f;
		noise_z = (2 * noise_x) - 1;

		Vector3 drift_offset{ noise_x, noise_y, noise_z };
		drift_offset = Transform::Unit3(drift_offset);
		drift_offset = drift_offset * this->drift_delta;

		this->drift_direction = this->drift_direction + drift_offset;
		this->drift_direction = Transform::Unit3(this->drift_direction);
		
		this->rigidbody.velocity += this->drift_direction * this->drift_size * Physics::DeltaTime();
		this->seed = noisy_bits;
	}

	long NextSample(long current) {
		long next = current * 1103515245 + 12345;
		next = (unsigned)(next / 65536) % 32768;

		return next;
	}
};