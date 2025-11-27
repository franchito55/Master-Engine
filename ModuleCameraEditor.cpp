#include "Globals.h"
#include "ModuleCameraEditor.h"
#include "Application.h"
#include "ModuleInput.h"
#include "Keyboard.h"

#define PI 3.14159265358979323846

ModuleCameraEditor::ModuleCameraEditor(HWND hWnd) {
	transform.position = Vector3(-5.0f, 3.0f, -5.0f);
	transform.rotation = Quaternion::Identity;
	transform.forward = Vector3(0.0f, 0.0f, 1.0f);
	transform.up = Vector3(0.0f, 1.0f, 0.0f);
	transform.right = transform.up.Cross(transform.forward);
}

bool ModuleCameraEditor::init() {
	app->setModuleCamera(this);

	return true;
}

void ModuleCameraEditor::update() {
	/*transform.position.x = imGuiCameraPos[0];
	transform.position.y = imGuiCameraPos[1];
	transform.position.z = imGuiCameraPos[2];*/

	Keyboard::State kbState = app->getModuleInput()->GetKeyboard()->GetState();
	if (kbState.D) {
		transform.position += transform.right * moveSpeed;
	}
	else if (kbState.A) {
		transform.position -= transform.right * moveSpeed;
	}

	if (kbState.W) {
		transform.position -= transform.forward * moveSpeed;
	}
	else if (kbState.S) {
		transform.position += transform.forward * moveSpeed;
	}
	if (kbState.E) {
		transform.position += transform.up * moveSpeed;
	}
	else if (kbState.Q) {
		transform.position -= transform.up * moveSpeed;
	}

	if (kbState.Up) {
		Quaternion rotationDelta = Quaternion::CreateFromAxisAngle(transform.right, rotationSpeed);
		transform.forward = Vector3::Transform(transform.forward, rotationDelta);
		transform.up = Vector3::Transform(transform.up, rotationDelta);
		recalculateRight();
	}
	else if (kbState.Down) {
		Quaternion rotationDelta = Quaternion::CreateFromAxisAngle(transform.right, -rotationSpeed);
		transform.forward = Vector3::Transform(transform.forward, rotationDelta);
		transform.up = Vector3::Transform(transform.up, rotationDelta);
		recalculateRight();
	}

	if (kbState.Left) {
		Quaternion rotationDelta = Quaternion::CreateFromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), rotationSpeed);
		transform.forward = Vector3::Transform(transform.forward, rotationDelta);
		transform.up = Vector3::Transform(transform.up, rotationDelta);
		recalculateRight();
	}
	else if (kbState.Right) {
		Quaternion rotationDelta = Quaternion::CreateFromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), -rotationSpeed);
		transform.forward = Vector3::Transform(transform.forward, rotationDelta);
		transform.up = Vector3::Transform(transform.up, rotationDelta);
		recalculateRight();
	}

	//view = Matrix::CreateWorld(transform.position, transform.forward, transform.up);
	view = Matrix::CreateLookAt(transform.position, transform.position - transform.forward, transform.up);
	projection = Matrix::CreatePerspectiveFieldOfView(fov * (PI / 180.0f), (float)app->getWindowWidth() / app->getWindowHeight(), nearPlane, farPlane);
}

void ModuleCameraEditor::render() {

	ImGui::Begin("Camera");
	if (ImGui::TreeNode("Position")) {
		ImGui::DragFloat("X", &transform.forward.x, 0.1f, -20.0f, 20.0f);
		ImGui::DragFloat("Y", &transform.forward.y, 0.1f, -20.0f, 20.0f);
		ImGui::DragFloat("Z", &transform.forward.z, 0.1f, -20.0f, 20.0f);
		ImGui::TreePop();
	}
	ImGui::DragFloat("FOV", &fov, 1.0f, 5.0f, 120.0f);
	if (ImGui::TreeNode("Forward")) {
		ImGui::DragFloat("X", &transform.forward.x, 0.1f, -20.0f, 20.0f);
		ImGui::DragFloat("Y", &transform.forward.y, 0.1f, -20.0f, 20.0f);
		ImGui::DragFloat("Z", &transform.forward.z, 0.1f, -20.0f, 20.0f);
		ImGui::TreePop();
	}
	ImGui::DragFloat("Move speed", &moveSpeed, 0.05f, 0.1f, 2.0f);
	ImGui::DragFloat("Rotation speed", &rotationSpeed, 0.05f, 0.1f, 2.0f);
	
	ImGui::End();

	ImGui::ShowDemoWindow();
}

void ModuleCameraEditor::recalculateRight() {
	transform.right = transform.up.Cross(transform.forward);
}