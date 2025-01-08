#pragma once

#include "../engine/RigidBody.h"

struct DroneAlpha {
	Rigidbody rigidbody;

	void Init() {
		this->rigidbody.Init();
	}
};