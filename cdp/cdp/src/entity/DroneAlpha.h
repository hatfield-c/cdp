#pragma once

#include "../engine/RigidBody.h"
#include "ai/Wallrider.h"

struct DroneAlpha {
	Rigidbody rigidbody;
	Wallrider wallrider;

	float forward_speed = 0.5;
	float forward_max = 1.5;

	float yaw_speed = 0.5;
	float yaw_max = 1;

	float roll_speed = 0.03;
	float roll_max = Math::Pi() / 6;

	float climb_speed = 0.3;
	float climb_max = 2;

	void Init() {
		this->rigidbody.Init();
		this->wallrider.Init();
	}

	void Update(float* depth_phash, Vector3* camera_cloud) {
		this->wallrider.Update(camera_cloud);

		Vector3 command = this->wallrider.GetCommand(depth_phash, camera_cloud);
		this->FollowCommand(command);
	}

	void FollowCommand(Vector3 command) {

		float yaw_delta = -command.x * this->yaw_speed;
		this->rigidbody.angular_velocity.y += yaw_delta;
		this->rigidbody.angular_velocity.y = Math::Clip(this->rigidbody.angular_velocity.y, -this->yaw_max, this->yaw_max);

		Vector3 body_forward = this->rigidbody.Forward();
		body_forward.y = 0;
		Vector3 acceleration = body_forward * this->forward_speed * command.z;
		
		acceleration.y += command.y * this->climb_speed;

		this->rigidbody.velocity += acceleration;
		this->rigidbody.velocity.y = Math::Clip(this->rigidbody.velocity.y, -this->climb_max, this->climb_max);

		Vector3 planar_velocity{ this->rigidbody.velocity.x, 0, this->rigidbody.velocity.z };
		float speed = Transform::Norm3(planar_velocity);

		if (speed > this->forward_max) {
			planar_velocity = Transform::Unit3(planar_velocity) * this->forward_max;

			this->rigidbody.velocity.x = planar_velocity.x;
			this->rigidbody.velocity.z = planar_velocity.z;
		}

		float roll_amount = this->roll_speed * command.x;
		Vector4 roll_delta = Quaternion::QuaternionFromEulerParams(this->rigidbody.Forward(), roll_amount);
		//this->rigidbody.rotation = Quaternion::MultiplyQuaternions(roll_delta, this->rigidbody.rotation, true);

		Vector3 right = this->rigidbody.Right();
		float xy_distance = Transform::Norm2(Vector2{right.x, right.z});
		float theta = Quaternion::Atan2(xy_distance, right.y);
		float theta_val = abs(theta);
		
		float roll_target = this->roll_max;
		float roll_sign = 1;

		if (theta < 0) {
			roll_sign = -1;
		}

		if (command.x == 0) {
			roll_target = theta_val * 0.95;
		}
		
		if (theta_val > roll_target) {
			roll_amount = (theta_val - roll_target) * roll_sign;
			roll_delta = Quaternion::QuaternionFromEulerParams(this->rigidbody.Forward(), roll_amount);

			//this->rigidbody.rotation = Quaternion::MultiplyQuaternions(roll_delta, this->rigidbody.rotation, true);
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