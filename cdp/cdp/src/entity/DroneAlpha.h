#pragma once

#include "../engine/RigidBody.h"

struct DroneAlpha {
	Rigidbody rigidbody;

	float forward_speed = 0.5;
	float forward_max = 1.5;

	float yaw_speed = 0.5;
	float yaw_max = 1;

	float roll_target = 45.0;

	float climb_speed = 0.3;
	float climb_max = 0.5;

	void Init() {
		this->rigidbody.Init();
	}

	void Command(Vector3 command) {
		float yaw_delta = -command.x * this->yaw_speed;
		this->rigidbody.angular_velocity.y += yaw_delta;
		this->rigidbody.angular_velocity.y = Math::Clip(this->rigidbody.angular_velocity.y, -this->yaw_max, this->yaw_max);

		Vector3 body_forward = this->rigidbody.Forward();
		body_forward.y = 0;
		Vector3 acceleration = body_forward * this->forward_speed * command.z;
		
		acceleration.y += command.y * this->climb_speed;
		acceleration.y = Math::Clip(acceleration.y, -this->climb_max, this->climb_max);

		this->rigidbody.velocity += acceleration;

		Vector3 planar_velocity{ this->rigidbody.velocity.x, 0, this->rigidbody.velocity.z };
		float speed = Transform::Norm3(planar_velocity);

		if (speed > this->forward_max) {
			planar_velocity = Transform::Unit3(planar_velocity) * this->forward_max;

			this->rigidbody.velocity.x = planar_velocity.x;
			this->rigidbody.velocity.z = planar_velocity.z;
		}

		if (command.x == 0) {
			this->rigidbody.angular_velocity.y *= 0.8;
		}

		if (command.y == 0) {
			this->rigidbody.velocity.y *= 0.8;
		}

		if (command.z == 0) {
			this->rigidbody.velocity.x *= 0.8;
			this->rigidbody.velocity.z *= 0.8;
		}
	}
};