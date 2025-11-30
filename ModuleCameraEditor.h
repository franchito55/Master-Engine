#pragma once
#include "Globals.h"
#include "Module.h"

struct Transform {
	Vector3 position = Vector3(0.0f);
	Quaternion rotation = Quaternion::Identity;
	Vector3 scale = Vector3(1.0f);

	Vector3 forward = Vector3(0.0f, 0.0f, -1.0f);
	Vector3 right = Vector3(1.0f, 0.0f, 0.0f);
	Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
};

class ModuleCameraEditor : public Module {
public:
	ModuleCameraEditor(HWND hWnd);
	Transform GetTransform() const { return transform; }
	Matrix GetViewMatrix() const { return view; }
	Matrix GetProjectionMatrix() const { return projection; }
	Vector3 getTarget() const { return target; }

	bool init() override;
	void update() override;
	void render() override;

	void recalculateRight();
	void resetState();

private:
	Transform transform = {};

	// Ideally a Transform* so we can focus on objects
	Vector3 target = Vector3(0.0f, 0.0f, 0.0f);

	Matrix view = {};
	Matrix projection = {};

	float fov = 90.0f;
	float nearPlane = 0.1f;
	float farPlane = 100.0f;

	float pitch = 0.0f;
	float yaw = 0.0f;

	const float pitchMin = -XM_PIDIV2 + 0.01f;
	const float pitchMax = XM_PIDIV2 - 0.01f;

	float orbitDistance = 5.0f;
	
	float moveSpeed = 0.1f;
	float rotationSpeed = 0.005f;
	float zoomSpeed = 0.005f;

	unsigned int previousMouseX = 0;
	unsigned int previousMouseY = 0;
	int previousScrollWheelValue = 0;
};