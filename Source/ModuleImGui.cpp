#include "Globals.h"
#include "ModuleImGui.h"
#include "ImGuiPass.h"
#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleAssignment2.h"
#include "ModuleCameraEditor.h"
#include "ModuleNonShaderDescriptors.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleInput.h"
#include "Keyboard.h"
#include "Mouse.h"

#define MAX_ORBITING_DISTANCE 30.0f
#define MIN_ORBITING_DISTANCE 0.3f
#define PI 3.14159265359
#define RAD2DEG 180.0f / PI
#define DEG2RAD PI / 180.0f

extern Application* app;

bool ModuleImGui::init() {
	app->setModuleImGui(this);
	app->setSceneRenderWindowWidth(400);
	app->setSceneRenderWindowHeight(400);
	moduleD3D12 = app->getModuleD3D12();
	moduleCamera = app->getModuleCamera();
	sceneRenderWindowSize = { 400, 400 };
	uiRotationDeg = app->getModuleAssignment2()->getObjectRotation()->ToEuler() * RAD2DEG;

	// ============ Init ImGui wrapper ============
	unsigned int descriptorsIndex = app->getModuleShaderDescriptors()->allocateDescriptor();
	imGuiPass = new ImGuiPass(moduleD3D12->getDevice().Get(), hWnd, app->getModuleShaderDescriptors()->getCPUHandleFromGenericHeap(descriptorsIndex), app->getModuleShaderDescriptors()->getGPUHandleFromGenericHeap(descriptorsIndex));

	return true;
}

void ModuleImGui::preRender() {
	// preRender de ModuleEditor
	imGuiPass->startFrame();
}

void ModuleImGui::render() {

	ImGui::DockSpaceOverViewport();
	
	showFpsInfoWindow();
	showTextureInfoWindow();
	showDebugGizmosWindow();
	showConsoleWindow();
	showCameraInfoWindow();
	showLightingInfoWindow();
	showMaterialInfoWindow();
	showSceneRenderWindow();
	showGeometryInfoWindow();

	fpsCount++;

	// This HAS to go last so that the UI gets rendered on top
	unsigned int rtvIndexInRTVHeap = app->getModuleD3D12()->getCurrentRTVIndexInRTVHeap();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuDescriptorHandle = app->getModuleNonShaderDescriptors()->getCPUHandleFromRTVHeap(rtvIndexInRTVHeap);
	imGuiPass->record(moduleD3D12->getCurrentBufferCommandList().Get(), rtvCpuDescriptorHandle); // TODO : change when we implement rendering to a texture
}

void ModuleImGui::showFpsInfoWindow() {
	// ============ FPS info window ============
	ImGui::Begin("FPS info");
	unsigned int index = fpsCount % FPS_PLOTTING_MAX;
	frameTimes[index] = moduleD3D12->getDeltaTime() / 10000.0f;
	fps[index] = 1000.0f / frameTimes[index];

	if (index == 0) {
		minFps = 99999;
		maxFps = 0;
	}
	if (fps[index] < minFps)
		minFps = fps[index];
	if (fps[index] > maxFps)
		maxFps = fps[index];
	unsigned int averageFps = 0;
	unsigned int numFps = 0;
	for (unsigned int i = 0; i < FPS_PLOTTING_MAX; i++) {
		if (fps[i] != 0 && fps[i] != INFINITE) {
			numFps++;
			averageFps += fps[i];
		}
	}
	averageFps /= numFps;

	char overlay[32];
	snprintf(overlay, 32, "avg: %d min: %d max: %d", averageFps, minFps, maxFps);
	ImGui::PlotLines("Frame times", frameTimes, IM_ARRAYSIZE(frameTimes), 0, (std::to_string((int)frameTimes[index]) + " ms").c_str(), 0.0f, 32.0f, ImVec2(0, 80.0f));
	ImGui::PlotLines("FPS", fps, IM_ARRAYSIZE(fps), 0, overlay, 0.0f, 360.0f, ImVec2(0, 80.0f));
	ImGui::End();
}

void ModuleImGui::showTextureInfoWindow() {
	// ============ Texture info window ============
	ImGui::Begin("Texture info");

	int prevFilteringMode = *app->getModuleAssignment2()->getCurrentTextureFilteringMode();
	const char* filteringModes[] = { "LINEAR", "POINT" };
	int* currentTextureFiltering = app->getModuleAssignment2()->getCurrentTextureFilteringMode();
	ImGui::Combo("Filtering mode", currentTextureFiltering, filteringModes, IM_ARRAYSIZE(filteringModes));
	if (*currentTextureFiltering != prevFilteringMode) // Set this flag to change texture filtering mode next frame
		app->getModuleAssignment2()->setTextureFilteringChanged(true);

	int prevAddressingMode = *app->getModuleAssignment2()->getCurrentTextureAddressingMode();
	const char* addressingModes[] = { "WRAP", "CLAMP" };
	int* currentTextureAddressingMode = app->getModuleAssignment2()->getCurrentTextureAddressingMode();
	ImGui::Combo("Addressing mode", currentTextureAddressingMode, addressingModes, IM_ARRAYSIZE(addressingModes));
	if (*currentTextureAddressingMode != prevAddressingMode) // Set this flag to change texture addressing mode next frame
		app->getModuleAssignment2()->setTextureAddressingChanged(true);

	ImGui::End();
}

void ModuleImGui::showGeometryInfoWindow() {
	// ============ Geometry info window ============
	ImGui::Begin("Geometry");
	Matrix cameraViewMatrix = moduleCamera->getViewMatrix();
	Matrix cameraProjectionMatrix = moduleCamera->getProjectionMatrix();
	Matrix* model = app->getModuleAssignment2()->getModelMatrix();

	if (ImGui::IsKeyPressed(ImGuiKey_T))
		mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	if (ImGui::IsKeyPressed(ImGuiKey_R))
		mCurrentGizmoOperation = ImGuizmo::ROTATE;
	if (ImGui::IsKeyPressed(ImGuiKey_E))
		mCurrentGizmoOperation = ImGuizmo::SCALE;
	if (ImGui::RadioButton("Translate [T]", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
		mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Rotate [R]", mCurrentGizmoOperation == ImGuizmo::ROTATE))
		mCurrentGizmoOperation = ImGuizmo::ROTATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Scale [E]", mCurrentGizmoOperation == ImGuizmo::SCALE))
		mCurrentGizmoOperation = ImGuizmo::SCALE;

	Vector3* position = app->getModuleAssignment2()->getObjectPosition();
	Vector3* scale = app->getModuleAssignment2()->getObjectScale();
	Vector3 rotationBefore = uiRotationDeg;
	ImGui::DragFloat3("Position", &position->x, 0.1f, -40.0f, 40.0f);
	ImGui::DragFloat3("Rotation", &uiRotationDeg.x, 1.0f, -360.0f, 360.0f);
	ImGui::DragFloat3("Scale", &scale->x, 0.001f, -10.0f, 10.0f);

	if (mCurrentGizmoOperation != ImGuizmo::SCALE)
	{
		if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
			mCurrentGizmoMode = ImGuizmo::LOCAL;
		ImGui::SameLine();
		if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
			mCurrentGizmoMode = ImGuizmo::WORLD;
	}
	if (ImGuizmo::IsUsing()) {
		float matrixTranslation[3], matrixRotation[3], matrixScale[3];
		ImGuizmo::DecomposeMatrixToComponents(&model->_11, matrixTranslation, matrixRotation, matrixScale);
		app->getModuleAssignment2()->setObjectPosition(Vector3(matrixTranslation[0], matrixTranslation[1], matrixTranslation[2]));
		app->getModuleAssignment2()->setObjectScale(Vector3(matrixScale[0], matrixScale[1], matrixScale[2]));
		Matrix deltaModelMatrix = modelMatrixBeforeGizmo.Invert() * *model;
		Quaternion deltaRotation = Quaternion::CreateFromRotationMatrix(deltaModelMatrix);
		Quaternion currRotation = *app->getModuleAssignment2()->getObjectRotation();
		Quaternion finalRotation;
		if (mCurrentGizmoMode == ImGuizmo::LOCAL)
			finalRotation = deltaRotation * currRotation;
		else
			finalRotation = currRotation * deltaRotation;
		finalRotation.Normalize();
		app->getModuleAssignment2()->setObjectRotation(finalRotation);
		uiRotationDeg = finalRotation.ToEuler() * RAD2DEG;
	}
	else if (uiRotationDeg != rotationBefore) {
		Vector3 newRotationDeg = uiRotationDeg * DEG2RAD;
		Quaternion newRotation = Quaternion::CreateFromYawPitchRoll(newRotationDeg.y, newRotationDeg.x, newRotationDeg.z);
		app->getModuleAssignment2()->setObjectRotation(newRotation);
	}

	ImGui::End();
}

void ModuleImGui::showDebugGizmosWindow() {
	ImGui::Begin("Debug Info");
	ImGui::Checkbox("Show XZ plane grid", &showXZGrid);
	ImGui::Checkbox("Show world origin axis triad", &showAxisTriad);
	ImGui::Checkbox("Show camera target position", &showCameraTarget);
	ImGui::Checkbox("Show geometry transform gizmo", &showGeometryGizmo);
	ImGui::End();

	if (showXZGrid)
		dd::xzSquareGrid(-20.0f, 20.0f, 0.0f, 1.0f, dd::colors::LightGray);
	if (showAxisTriad) {
		// To avoid z-fighting between axis and grid. Axis lines always drawn on top
		Vector3 nudge = moduleCamera->getTransform().position;
		nudge.Normalize();
		Matrix axisPos = Matrix::CreateTranslation(nudge * 0.001f);
		dd::axisTriad(ddConvert(axisPos), 0.05f, 0.5f);
	}
	if (showCameraTarget) {
		float cameraTargetColor[3] = { 1.0f, 0.0f, 0.0f };
		Vector3 cameraTarget = moduleCamera->getTarget();
		dd::sphere(&cameraTarget.x, cameraTargetColor, 0.025f);
	}
}

void ModuleImGui::log(const char* t) {
	consoleLog.emplace_back(t);
}

void ModuleImGui::showConsoleWindow() {
	// ============ Output console window ============
	ImGui::Begin("Console");
	if (ImGui::Button("Clear")) {
		consoleLog.clear();
	}
	ImGui::SameLine();
	if (ImGui::Button("Copy")) {
		ImGui::LogToClipboard();
		for (const auto& line : consoleLog) ImGui::TextUnformatted(line.c_str());
		ImGui::LogFinish();
	}
	ImGui::Separator();
	ImGui::BeginChild("Log", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
	for (const auto& line : consoleLog) {
		ImGui::TextUnformatted(line.c_str());
	}

	if (scrollConsoleToBottom) {
		ImGui::SetScrollHereY(1.0f);
		scrollConsoleToBottom = false;
	}
	ImGui::EndChild();
	ImGui::End();
}

void ModuleImGui::showCameraInfoWindow() {
	Vector3 cameraPos = moduleCamera->getTransform().position;
	Vector3 cameraForward = moduleCamera->getTransform().forward;
	Vector3 cameraUp = moduleCamera->getTransform().up;
	Vector3 cameraTarget = moduleCamera->getTarget();
	float cameraOrbitingDist = moduleCamera->getCurrentOrbitingDistance();

	Vector3 newPos = cameraPos;
	Vector3 newForward = cameraForward;
	Vector3 newUp = cameraUp;
	Vector3 newTarget = cameraTarget;
	float newOrbitingDist = cameraOrbitingDist;

	ImGui::Begin("Camera");
	if (ImGui::CollapsingHeader("Vectors")) {
		ImGui::DragFloat3("Position", &newPos.x, 0.1f, -20.0f, 20.0f);
		ImGui::DragFloat3("Forward", &newForward.x, 0.1f, -20.0f, 20.0f);
		ImGui::DragFloat3("Up", &newUp.x, 0.1f, -20.0f, 20.0f);
		ImGui::DragFloat3("Target", &newTarget.x, 0.1f, -20.0f, 20.0f);
		ImGui::DragFloat("Distance", &newOrbitingDist, 0.1f, MIN_ORBITING_DISTANCE, MAX_ORBITING_DISTANCE);
	}
	if (ImGui::CollapsingHeader("Parameters")) {
		ImGui::DragFloat("FOV", moduleCamera->getFov(), 1.0f, 5.0f, 120.0f);
		ImGui::DragFloat("Move speed", moduleCamera->getMoveSpeed(), 1.0f, 5.0f, 100.0f);
		ImGui::DragFloat("Rotation speed", moduleCamera->getRotationSpeed(), 0.1f, 1.0f, 20.0f);
		ImGui::DragFloat("Zoom speed", moduleCamera->getZoomSpeed(), 0.1f, 1.0f, 20.0f);
	}
	ImGui::End();

	// Check if edited via ImGui and set a flag in ModuleCameraEditor
	if (newPos != cameraPos) {
		moduleCamera->setPosUpdatedViaImGui(true);
		moduleCamera->setImGuiPos(newPos);
	}
	if (newForward != cameraForward) {
		moduleCamera->setForwardUpdatedViaImGui(true);
		moduleCamera->setImGuiForward(newForward);
	}
	if (newUp != cameraUp) {
		moduleCamera->setUpUpdatedViaImGui(true);
		moduleCamera->setImGuiUp(newUp);
	}
	if (newTarget != cameraTarget) {
		moduleCamera->setTargetUpdatedViaImGui(true);
		moduleCamera->setImGuiTarget(newTarget);
	}
	if (newOrbitingDist != cameraOrbitingDist) {
		moduleCamera->setOrbitingDistanceUpdatedViaImGui(true);
		moduleCamera->setImGuiOrbitingDistance(newOrbitingDist);
	}
}

void ModuleImGui::showLightingInfoWindow() {
	Vector3* pbrLightPosition = app->getModuleAssignment2()->getLightPosition();
	Vector3* pbrLightColor = app->getModuleAssignment2()->getLightColor();
	float* pbrLightIntensity = app->getModuleAssignment2()->getLightIntensity();

	ImGui::Begin("Light");
	ImGui::DragFloat3("Light position", &pbrLightPosition->x, 0.1f, -40.0f, 40.0f);
	ImGui::ColorEdit3("Light color", &pbrLightColor->x);
	ImGui::DragFloat("Light intensity", pbrLightIntensity, 0.05f, 0.5f, 5.0f);
	ImGui::End();
}

void ModuleImGui::showMaterialInfoWindow() {
	Vector3* pbrMaterialDiffuse = app->getModuleAssignment2()->getMaterialDiffuse();
	Vector3* pbrMaterialRf0 = app->getModuleAssignment2()->getMaterialFresnel0();
	float* pbrMaterialN = app->getModuleAssignment2()->getMaterialN();

	ImGui::Begin("Material");
	ImGui::ColorEdit3("Material diffuse", &pbrMaterialDiffuse->x);
	ImGui::ColorEdit3("Material Rf0", &pbrMaterialRf0->x, 0.01f);
	ImGui::DragFloat("Material n", pbrMaterialN, 0.5f, 1.0f, 1500.0f);
	ImGui::End();
}

void ModuleImGui::showSceneRenderWindow() {
	ImGui::Begin("Scene");
	ImVec2 newWindowSize = ImGui::GetContentRegionAvail();
	if (newWindowSize.x > 0 && newWindowSize.y > 0 &&
		(abs(sceneRenderWindowSize.x - newWindowSize.x) > 1 ||
			abs(sceneRenderWindowSize.y - newWindowSize.y) > 1)) {
		app->setSceneRenderWindowWidth(newWindowSize.x);
		app->setSceneRenderWindowHeight(newWindowSize.y);
		sceneRenderWindowSize = newWindowSize;
		RECT resizePending;
		resizePending.left = 0;
		resizePending.right = sceneRenderWindowSize.x;
		resizePending.top = 0;
		resizePending.bottom = sceneRenderWindowSize.y;
		app->getModuleD3D12()->setSceneResizePending(resizePending);
	}

	sceneRenderWindowPos = ImGui::GetWindowPos();
	sceneRenderWindowCursorPos = ImGui::GetCursorPos();
	sceneRenderWindowImageRectMin = ImGui::GetWindowContentRegionMin();
	sceneRenderWindowImageRectMax = ImGui::GetWindowContentRegionMax();

	sceneRenderWindowHovered = ImGui::IsWindowHovered();
	ImGui::Image((ImTextureID)app->getModuleShaderDescriptors()->getGPUHandleFromGenericHeap(app->getModuleD3D12()->getSceneSRVIndexInHeap()).ptr, sceneRenderWindowSize);

	// Gizmo
	Matrix cameraViewMatrix = moduleCamera->getViewMatrix();
	Matrix cameraProjectionMatrix = moduleCamera->getProjectionMatrix();
	Matrix* model = app->getModuleAssignment2()->getModelMatrix();
	ImGuizmo::SetRect(
		sceneRenderWindowPos.x + sceneRenderWindowImageRectMin.x,
		sceneRenderWindowPos.y + sceneRenderWindowImageRectMin.y,
		sceneRenderWindowImageRectMax.x - sceneRenderWindowImageRectMin.x,
		sceneRenderWindowImageRectMax.y - sceneRenderWindowImageRectMin.y
	);
	ImGuizmo::BeginFrame();
	ImGuizmo::SetDrawlist();
	modelMatrixBeforeGizmo = *model;
	ImGuizmo::Manipulate(&cameraViewMatrix._11, &cameraProjectionMatrix._11, mCurrentGizmoOperation, mCurrentGizmoMode, &model->_11, NULL, NULL);

	ImGui::End();
}

bool ModuleImGui::compareVectors(float* v0, float* v1) {
	return v0[0] == v1[0] && v0[1] == v1[1] && v0[2] == v1[2];
}