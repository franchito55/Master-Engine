#include "Globals.h"
#include "ModuleCameraEditor.h"
#include "Application.h"
#include "ModuleInput.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "DebugDrawPass.h"
#include "ModuleAssignment2.h"
#include "ModuleImGui.h"
#include <algorithm>

#define PI 3.14159265358979323846
#define MAX_ORBITING_DISTANCE 30.0f
#define MIN_ORBITING_DISTANCE 0.3f

ModuleCameraEditor::ModuleCameraEditor(HWND hWnd) {}

bool ModuleCameraEditor::init() {
	app->setModuleCamera(this); transform.position = Vector3(1.0f, 1.5f, 3.0f);
	transform.rotation = Quaternion::Identity;
	transform.forward = target - transform.position;
	transform.forward.Normalize();
	transform.up = Vector3(0.0f, 1.0f, 0.0f);
	transform.right = transform.forward.Cross(transform.up);
	currentOrbitingDistance = (target - transform.position).Length();

	imGuiPos = transform.position;
	imGuiForward = transform.forward;
	imGuiUp = transform.up;
	imGuiTarget = target;
	imGuiOrbitingDistance = currentOrbitingDistance;

	return true;
}

void ModuleCameraEditor::update() {

	Keyboard::State kbState = app->getModuleInput()->GetKeyboard()->GetState();

	float realMoveSpeed = moveSpeed;
	if (kbState.LeftShift) {
		realMoveSpeed *= 2.0f;
	}

	Mouse::State mouseState = app->getModuleInput()->GetMouse()->GetState();
	int mouseDeltaX = mouseState.x - previousMouseX;
	int mouseDeltaY = mouseState.y - previousMouseY;
	int mouseScrollWheelDelta = mouseState.scrollWheelValue - previousScrollWheelValue;

	// Don't scroll if hovering any ImGui window
	if (app->getModuleImGui()->getIsSceneRenderWindowHovered() && mouseScrollWheelDelta != 0) {
		// We have to check if the next distance is <= the current one, since if the user scrolls really hard, it could jump to
		// the other side of the triangle, bypassing the max zoom
		Vector3 nextPos = transform.position + transform.forward * zoomSpeed / 1000.0f * mouseScrollWheelDelta;
		float nextDistance = (nextPos - target).Length();
		if ((mouseScrollWheelDelta > 0 && !anySignsDiffer(transform.position - target, nextPos - target) && nextDistance >= MIN_ORBITING_DISTANCE) || (mouseScrollWheelDelta < 0 && nextDistance <= MAX_ORBITING_DISTANCE)) {
			transform.position = nextPos;
			currentOrbitingDistance = (target - transform.position).Length();
		}
	}

	Vector3 targetOffset = target - transform.position;

	// Don't rotate if modifying the ImGui's drag sliders
	if (app->getModuleImGui()->getIsSceneRenderWindowHovered() && kbState.LeftAlt && mouseState.leftButton && !mouseState.rightButton) {

		if (mouseDeltaY != 0) {
			Vector3 offset = transform.position - target;
			Quaternion rotationDelta = Quaternion::CreateFromAxisAngle(transform.right, -rotationSpeed / 1000.0f * mouseDeltaY);
			transform.forward = Vector3::Transform(transform.forward, rotationDelta);
			transform.up = Vector3::Transform(transform.up, rotationDelta);
			Vector3 rotatedOffset = Vector3::Transform(offset, rotationDelta);
			transform.position = target + rotatedOffset;
			transform.right = transform.forward.Cross(transform.up);
		}

		if (mouseDeltaX != 0) {
			Vector3 offset = transform.position - target;
			Quaternion rotationDelta = Quaternion::CreateFromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), -rotationSpeed / 1000.0f * mouseDeltaX);
			transform.forward = Vector3::Transform(transform.forward, rotationDelta);
			transform.up = Vector3::Transform(transform.up, rotationDelta);
			Vector3 rotatedOffset = Vector3::Transform(offset, rotationDelta);
			transform.position = target + rotatedOffset;
			transform.right = transform.forward.Cross(transform.up);
		}
	}

	// Only EITHER rotate OR orbit, not both (too complicated and too tired to think of it)
	if (app->getModuleImGui()->getIsSceneRenderWindowHovered() && mouseState.rightButton && !mouseState.leftButton) {
		// Get the offset of the target relative to the camera
		Vector3 offset = target - transform.position;

		if (mouseDeltaY != 0) {
			Quaternion rotY = Quaternion::CreateFromAxisAngle(transform.right, -rotationSpeed / 1000.0f * mouseDeltaY);

			// rotate orientation using rotY
			transform.forward = Vector3::Transform(transform.forward, rotY);
			transform.up = Vector3::Transform(transform.up, rotY);

			// rotate offset first
			offset = Vector3::Transform(offset, rotY);
		}

		if (mouseDeltaX != 0) {
			Quaternion rotX = Quaternion::CreateFromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), -rotationSpeed / 1000.0f * mouseDeltaX);

			// rotate orientation using rotX
			transform.forward = Vector3::Transform(transform.forward, rotX);
			transform.up = Vector3::Transform(transform.up, rotX);

			// rotate offset first
			offset = Vector3::Transform(offset, rotX);
		}

		// Normalize everything just in case
		transform.forward.Normalize();
		transform.up.Normalize();
		transform.right = transform.forward.Cross(transform.up);
		transform.right.Normalize();

		// Compute the movement difference
		Vector3 movementDelta = Vector3::Zero;
		if (kbState.D)       
			movementDelta += transform.right * realMoveSpeed / 1000.0f;
		else if (kbState.A)  movementDelta -= transform.right * realMoveSpeed / 1000.0f;

		if (kbState.Space)   movementDelta += transform.up * realMoveSpeed / 1000.0f;
		else if (kbState.LeftControl) movementDelta -= transform.up * realMoveSpeed / 1000.0f;

		if (kbState.W)       movementDelta += transform.forward * realMoveSpeed / 1000.0f;
		else if (kbState.S)  movementDelta -= transform.forward * realMoveSpeed / 1000.0f;

		transform.position += movementDelta;

		// ONLY THEN update the target's position
		target = transform.position + offset;
		currentOrbitingDistance = (transform.position - target).Length();
	}

	if (app->getModuleImGui()->getIsSceneRenderWindowHovered() && kbState.F) {
		Vector3 newTarget = app->getModuleAssignment2()->getTransform()->position;
		transform.forward = newTarget - transform.position;
		transform.forward.Normalize();
		transform.right = transform.forward.Cross(Vector3(0.0f, 1.0f, 0.0f));
		transform.right.Normalize();
		transform.up = transform.right.Cross(transform.forward);
		transform.up.Normalize();
		target = newTarget;
		currentOrbitingDistance = (transform.position - target).Length();
	}

	previousMouseX = mouseState.x;
	previousMouseY = mouseState.y;
	previousScrollWheelValue = mouseState.scrollWheelValue;

	view = Matrix::CreateLookAt(transform.position, transform.position + transform.forward, transform.up);
	projection = Matrix::CreatePerspectiveFieldOfView(fov * (PI / 180.0f), (float)app->getSceneRenderWindowWidth() / app->getSceneRenderWindowHeight(), nearPlane, farPlane);
}

void ModuleCameraEditor::render() {

	// Recalculate target in case the user has changed the camera's values via ImGui
	if (posUpdatedViaImGui) {
		target = imGuiPos + transform.forward * currentOrbitingDistance;
		transform.position = imGuiPos;

		posUpdatedViaImGui = false;
	}
	else if (forwardUpdatedViaImGui) {
		imGuiForward.Normalize();
		transform.forward = imGuiForward;
		transform.right = transform.forward.Cross(transform.up);
		target = transform.position + transform.forward * currentOrbitingDistance;
		forwardUpdatedViaImGui = false;
	}
	else if (upUpdatedViaImGui) {
		imGuiUp.Normalize();
		transform.up = imGuiUp;
		transform.right = transform.forward.Cross(transform.up);
		upUpdatedViaImGui = false;
	}
	else if (targetUpdatedViaImGui) {
		transform.forward = imGuiTarget - transform.position;
		transform.forward.Normalize();
		transform.right = transform.forward.Cross(Vector3(0.0f, 1.0f, 0.0f));
		transform.right.Normalize();
		transform.up = transform.right.Cross(transform.forward);
		transform.up.Normalize();
		target = imGuiTarget;
		currentOrbitingDistance = (transform.position - target).Length();

		targetUpdatedViaImGui = false;
	}
	else if (orbitingDistanceUpdatedViaImGui) {
		currentOrbitingDistance = imGuiOrbitingDistance;
		Vector3 nudge = transform.position - target;
		nudge.Normalize();
		transform.position = target + nudge * currentOrbitingDistance;


		orbitingDistanceUpdatedViaImGui = false;
	}

    // --- Update view & projection matrices for this frame ---
    view = Matrix::CreateLookAt(transform.position, target, transform.up);
    projection = Matrix::CreatePerspectiveFieldOfView(
        fov * (PI / 180.0f),
        static_cast<float>(app->getSceneRenderWindowWidth()) / app->getSceneRenderWindowHeight(),
        nearPlane,
        farPlane
    );
}

void ModuleCameraEditor::resetState() {
	transform.position = Vector3(0.0f, 3.0f, 7.0f);
	transform.rotation = Quaternion::Identity;
	transform.forward = Vector3(0.0f, 0.0f, -1.0f);
	transform.up = Vector3(0.0f, 1.0f, 0.0f);
	transform.right = transform.forward.Cross(transform.up);
}

bool ModuleCameraEditor::anySignsDiffer(Vector3 position1, Vector3 position2) {
	return ((0 > position1.x) != (0 > position2.x)
		|| (0 > position1.y) != (0 > position2.y)
		|| (0 > position1.z) != (0 > position2.z));
}