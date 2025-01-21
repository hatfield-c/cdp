#pragma once

#include "Physics.h"
#include "Transform.h"
#include "Quaternion.h"

struct Rigidbody {
	float mass = 1;
	Vector3 position = Vector::ZERO3();
	Vector4 rotation{ 0, 0, 0, 1 };
	Vector3 velocity = Vector::ZERO3();
	Vector3 angular_velocity = Vector::ZERO3();

	void Init() {

	}

	void Update() {
		this->position += this->velocity * Physics::DeltaTime();

		Vector4 quaternion_delta = Quaternion::QuaternionFromEulerAngles(this->angular_velocity * Physics::DeltaTime());
		this->rotation = Quaternion::MultiplyQuaternions(quaternion_delta, this->rotation, true);
	}

	void Accelerate(Vector3 acceleration) {
		this->velocity += acceleration * Physics::DeltaTime();
	}

	void AddForce(Vector3 force) {
		this->Accelerate(force / this->mass);
	}

	void AddTorque(Vector3 torque) {
		this->angular_velocity += torque * Physics::DeltaTime();
	}

	void AirResistance(Vector3 wind) {
		this->AddTorque(this->angular_velocity * -0.000617f);

		Vector3 velocity_drag_force = this->velocity * - 0.5 * (1.293e-3) * 0.47 * Math::Pi() * (0.25 * 0.25);
		Vector3 wind_drag_force = wind * -0.5 * (1.293e-3) * 0.47 * Math::Pi() * (0.25 * 0.25);

		this->AddForce(velocity_drag_force);
		this->AddForce(wind_drag_force);
	}
};