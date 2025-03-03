#pragma once

struct Pid {
	float p_scale = 1;
	float i_scale = 0;
	float d_scale = 0;
	float i_max = 2;
	float d_max = 100000000;
	float integral = 0;
	float old_value = 0;
	float gradient_estimate = 0;

	float ControlStep(float current, float target, float gradient, bool use_estimate = false) {
		if (use_estimate) {
			gradient = this->gradient_estimate;
		}

		float error = target - current;

		this->integral += error;
		if (this->integral > this->i_max) {
			this->integral = this->i_max;
		}
		if (this->integral < -this->i_max) {
			this->integral = -this->i_max;
		}

		float p = error * this->p_scale;
		float i = this->integral * this->i_scale;
		float d = gradient * this->d_scale;

		if (d > this->d_max) {
			d = this->d_max;
		}
		if (d < -this->d_max) {
			d = -this->d_max;
		}

		float pid = p + i - d;

		float delta = current - this->old_value;
		this->gradient_estimate = ((1 / 3) * (this->gradient_estimate)) + ((2 / 3) * (delta));
		this->old_value = current;

		return pid;
	}
};