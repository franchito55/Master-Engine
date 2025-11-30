#include "Globals.h"
#include "ModuleCameraEditor.h"
#include "Application.h"
#include "ModuleInput.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "DebugDrawPass.h"

#define PI 3.14159265358979323846

ModuleCameraEditor::ModuleCameraEditor(HWND hWnd) {
	transform.position = Vector3(0.0f, 0.0f, 5.0f);
	transform.rotation = Quaternion::Identity;
	transform.forward = Vector3(0.0f, 0.0f, -1.0f);
	transform.up = Vector3(0.0f, 1.0f, 0.0f);
	transform.right = transform.up.Cross(transform.forward);
}

bool ModuleCameraEditor::init() {
	app->setModuleCamera(this);

	return true;
}

void ModuleCameraEditor::lateUpdate() {

	Mouse::State mouseState = app->getModuleInput()->GetMouse()->GetState();
	int mouseDeltaX = mouseState.x - previousMouseX;
	int mouseDeltaY = mouseState.y - previousMouseY;
	int mouseScrollWheelDelta = mouseState.scrollWheelValue - previousScrollWheelValue;

	if (mouseScrollWheelDelta != 0) {
		transform.position += transform.forward * zoomSpeed * mouseScrollWheelDelta;
	}

	if (!ImGui::IsAnyItemActive()) {
		if (mouseState.leftButton && mouseDeltaY != 0) {
			Quaternion rotationDelta = Quaternion::CreateFromAxisAngle(transform.right, -rotationSpeed * mouseDeltaY);
			transform.forward = Vector3::Transform(transform.forward, rotationDelta);
			transform.up = Vector3::Transform(transform.up, rotationDelta);
			recalculateRight();

			Vector3 targetOffset = target - transform.position;
			Quaternion targetRotationDelta = Quaternion::CreateFromAxisAngle(transform.right, -rotationSpeed * mouseDeltaY);
			Vector3 rotatedOffset = Vector3::Transform(targetOffset, rotationDelta);
			target = transform.position + rotatedOffset;
		}

		if (mouseState.leftButton && mouseDeltaX != 0) {
			Quaternion rotationDelta = Quaternion::CreateFromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), -rotationSpeed * mouseDeltaX);
			transform.forward = Vector3::Transform(transform.forward, rotationDelta);
			transform.up = Vector3::Transform(transform.up, rotationDelta);
			recalculateRight();

			Vector3 targetOffset = target - transform.position;
			Quaternion targetRotationDelta = Quaternion::CreateFromAxisAngle(transform.right, -rotationSpeed * mouseDeltaX);
			Vector3 rotatedOffset = Vector3::Transform(targetOffset, rotationDelta);
			target = transform.position + rotatedOffset;
		}

		if (mouseState.rightButton && mouseDeltaY != 0) {
			Vector3 offset = transform.position - target;
			Quaternion rotationDelta = Quaternion::CreateFromAxisAngle(transform.right, -rotationSpeed * mouseDeltaY);
			transform.forward = Vector3::Transform(transform.forward, rotationDelta);
			transform.up = Vector3::Transform(transform.up, rotationDelta);
			Vector3 rotatedOffset = Vector3::Transform(offset, rotationDelta);
			transform.position = target + rotatedOffset;
			recalculateRight();
		}

		if (mouseState.rightButton && mouseDeltaX != 0) {
			Vector3 offset = transform.position - target;
			Quaternion rotationDelta = Quaternion::CreateFromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), -rotationSpeed * mouseDeltaX);
			transform.forward = Vector3::Transform(transform.forward, rotationDelta);
			transform.up = Vector3::Transform(transform.up, rotationDelta);
			Vector3 rotatedOffset = Vector3::Transform(offset, rotationDelta);
			transform.position = target + rotatedOffset;
			recalculateRight();
		}
	}

	previousMouseX = mouseState.x;
	previousMouseY = mouseState.y;
	previousScrollWheelValue = mouseState.scrollWheelValue;


	Keyboard::State kbState = app->getModuleInput()->GetKeyboard()->GetState();
	if (kbState.D) {
		transform.position += transform.right * moveSpeed;
		target += transform.right * moveSpeed;
	}
	else if (kbState.A) {
		transform.position -= transform.right * moveSpeed;
		target -= transform.right * moveSpeed;
	}
	if (kbState.E) {
		transform.position += transform.up * moveSpeed;
		target += transform.up * moveSpeed;
	}
	else if (kbState.Q) {
		transform.position -= transform.up * moveSpeed;
		target -= transform.up * moveSpeed;
	}

	view = Matrix::CreateLookAt(transform.position, transform.position + transform.forward, transform.up);
	projection = Matrix::CreatePerspectiveFieldOfView(fov * (PI / 180.0f), (float)app->getWindowWidth() / app->getWindowHeight(), nearPlane, farPlane);
}

void ModuleCameraEditor::render() {

	ImGui::Begin("Camera");
	if (ImGui::CollapsingHeader("Vectors")) {
		ImGui::DragFloat3("Position", &transform.position.x, 0.1f, -20.0f, 20.0f);
		ImGui::DragFloat3("Forward", &transform.forward.x, 0.1f, -20.0f, 20.0f);
		ImGui::DragFloat3("Up", &transform.up.x, 0.1f, -20.0f, 20.0f);
		ImGui::DragFloat3("Target", &target.x, 0.1f, -20.0f, 20.0f);
	}
	if (ImGui::CollapsingHeader("Parameters")) {
		ImGui::DragFloat("FOV", &fov, 1.0f, 5.0f, 120.0f);
		ImGui::DragFloat("Move speed", &moveSpeed, 0.005f, 0.005f, 2.0f);
		ImGui::DragFloat("Rotation speed", &rotationSpeed, 0.005f, 0.005f, 2.0f);
		ImGui::DragFloat("Zoom speed", &zoomSpeed, 0.005f, 0.005f, 2.0f);
	}
	ImGui::End();

	ImGui::ShowDemoWindow();

	float color[3] = { 1.0f, 0.0f, 0.0f };
	float ddTarget[3] = { target.x, target.y, target.z };
	dd::sphere(ddTarget, color, 0.05f);
}

void ModuleCameraEditor::recalculateRight() {
	transform.right = transform.forward.Cross(transform.up);
}

void ModuleCameraEditor::resetState() {
	transform.position = Vector3(0.0f, 3.0f, 7.0f);
	transform.rotation = Quaternion::Identity;
	transform.forward = Vector3(0.0f, 0.0f, -1.0f);
	transform.up = Vector3(0.0f, 1.0f, 0.0f);
	recalculateRight();
}