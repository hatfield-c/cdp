#pragma once

#include "../../system/CudaError.h"

#include "IhmGenerator.h"
#include "IhmState.h"
#include "../WorldSpace.h"
#include "../Transform.h"
#include "../Quaternion.h"
#include "../Indexer.h"

struct IhmRenderer {

	Vector2 render_size;
	Vector2 render_size_strided;
	Vector2 render_stride;
	unsigned long long pixel_count;

	void Init(Vector2 render_size, Vector2 render_stride) {
		this->render_size = render_size;
		this->render_stride = render_stride;

		this->render_size_strided = Vector2{ (float)(int)(render_size.x / render_stride.x), (float)(int)(render_size.y / render_stride.y)};

		this->pixel_count = (unsigned long long)(render_size.x * render_size.y);
	}

	

};