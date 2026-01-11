#pragma once
#include "Globals.h"
#include "Module.h"
#include "structs/Transform.h"

struct Transform;

class ModuleCameraEditor : public Module {
public:
	ModuleCameraEditor(HWND hWnd);
	Transform getTransform() const { return transform; }
	Matrix getViewMatrix() const { return view; }
	Matrix getProjectionMatrix() const { return projection; }
	Vector3 getTarget() const { return target; }
	float getCurrentOrbitingDistance() { return currentOrbitingDistance; }
	float* getFov() { return &fov; }
	float* getMoveSpeed() { return &moveSpeed; }
	float* getRotationSpeed() { return &rotationSpeed; }
	float* getZoomSpeed() { return &zoomSpeed; }

	bool init() override;
	void update() override;
	void render() override;

	void resetState();

	void setPosUpdatedViaImGui(const bool updated) { posUpdatedViaImGui = updated; }
	void setForwardUpdatedViaImGui(const bool updated) { forwardUpdatedViaImGui = updated; }
	void setUpUpdatedViaImGui(const bool updated) { upUpdatedViaImGui = updated; }
	void setTargetUpdatedViaImGui(const bool updated) { targetUpdatedViaImGui = updated; }
	void setOrbitingDistanceUpdatedViaImGui(const bool updated) { orbitingDistanceUpdatedViaImGui = updated; }
	Vector3* getImGuiPos() { return &imGuiPos; }
	void setImGuiPos(Vector3 _imGuiPos) { imGuiPos = _imGuiPos; }
	Vector3* getImGuiForward() { return &imGuiForward; }
	void setImGuiForward(Vector3 _imGuiForward) { imGuiForward = _imGuiForward; }
	Vector3* getImGuiUp() { return &imGuiUp; }
	void setImGuiUp(Vector3 _imGuiUp) { imGuiUp = _imGuiUp; }
	Vector3* getImGuiTarget() { return &imGuiTarget; }
	void setImGuiTarget(Vector3 _imGuiTarget) { imGuiTarget = _imGuiTarget; }
	float* getImGuiOrbitingDistance() { return &imGuiOrbitingDistance; }
	void setImGuiOrbitingDistance(float _imGuiOrbitingDistance) { imGuiOrbitingDistance = _imGuiOrbitingDistance; }

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

	float currentOrbitingDistance = 0.0f;
	
	float moveSpeed = 30.0f;
	float rotationSpeed = 5.0f;
	float zoomSpeed = 5.0f;

	unsigned int previousMouseX = 0;
	unsigned int previousMouseY = 0;
	int previousScrollWheelValue = 0;

	bool posUpdatedViaImGui = false;
	bool forwardUpdatedViaImGui = false;
	bool upUpdatedViaImGui = false;
	bool targetUpdatedViaImGui = false;
	bool orbitingDistanceUpdatedViaImGui = false;

	// Need to keep duplicates of data to handle bidirectional editing via ImGui
	Vector3 imGuiPos;
	Vector3 imGuiForward;
	Vector3 imGuiUp;
	Vector3 imGuiTarget;
	float imGuiOrbitingDistance = 0.0f;

	bool anySignsDiffer(Vector3 target, Vector3 position);
};