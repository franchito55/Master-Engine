#pragma once
#include "Globals.h"
#include "Module.h"

struct Transform {
	Vector3 position = Vector3(0.0f);
	Quaternion rotation = Quaternion::Identity;
	Vector3 scale = Vector3(1.0f);

	Vector3 forward = Vector3(0.0f, 0.0f, 1.0f);
	Vector3 right = Vector3(1.0f, 0.0f, 0.0f);
	Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
};

class ModuleCameraEditor : public Module {
public:
	ModuleCameraEditor(HWND hWnd);
	Transform GetTransform() const { return transform; }
	Matrix GetViewMatrix() const { return view; }
	Matrix GetProjectionMatrix() const { return projection; }

	bool init() override;
	void update() override;
	void render() override;

	void recalculateRight();

private:
	Transform transform;

	Matrix view;
	Matrix projection;

	float fov = 90.0f;
	float nearPlane = 0.1f;
	float farPlane = 100.0f;
	
	float moveSpeed = 1.0f;
	float rotationSpeed = 1.0f;
};