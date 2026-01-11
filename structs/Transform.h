#pragma once
#include "Globals.h"

struct Transform {
	Vector3 position = Vector3(0.0f);
	Quaternion rotation = Quaternion::Identity;
	Vector3 scale = Vector3(1.0f);

	Vector3 forward = Vector3(0.0f, 0.0f, -1.0f);
	Vector3 right = Vector3(1.0f, 0.0f, 0.0f);
	Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
};