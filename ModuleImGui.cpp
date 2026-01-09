#include "Globals.h"
#include "ModuleImGui.h"
#include "ImGuiPass.h"
#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleAssignment2.h"
#include "ModuleCameraEditor.h"
#include "ImGuizmo.h"

extern Application* app;

bool ModuleImGui::init() {
	moduleD3D12 = app->getModuleD3D12();

	// ============ Init ImGui wrapper ============
	imGuiPass = new ImGuiPass(moduleD3D12->getDevice().Get(), hWnd, {0}, {0});

	return true;
}

void ModuleImGui::preRender() {
	// preRender de ModuleEditor
	imGuiPass->startFrame();
}

void ModuleImGui::render() {
	
	showFpsInfoWindow();
	showTextureInfoWindow();
	showGeometryInfoWindow();
	showDebugGizmosWindow();

	fpsCount++;

	// This HAS to go last so that the UI gets rendered on top
	imGuiPass->record(moduleD3D12->getCurrentBufferCommandList().Get(), *moduleD3D12->getCurrentRtvCpuDescriptorHandle()); // TODO : change when we implement rendering to a texture
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
	int* currentTextureAddressingMode = app->getModuleAssignment2()->getCurrentTextureFilteringMode();
	ImGui::Combo("Addressing mode", currentTextureAddressingMode, addressingModes, IM_ARRAYSIZE(addressingModes));
	if (*currentTextureAddressingMode != prevAddressingMode) // Set this flag to change texture addressing mode next frame
		app->getModuleAssignment2()->setTextureAddressingChanged(true);

	ImGui::End();
}

void ModuleImGui::showGeometryInfoWindow() {
	// ============ Geometry info window ============
	ImGui::Begin("Geometry");
	Matrix cameraViewMatrix = app->getModuleCamera()->GetViewMatrix();
	Matrix cameraProjMatrix = app->getModuleCamera()->GetProjectionMatrix();
	Matrix* model = app->getModuleAssignment2()->getModelMatrix();
	ImGuizmo::BeginFrame();
	handleEditTransform(&cameraViewMatrix._11, &cameraProjMatrix._11, &model->_11);
	ImGui::End();

	ImGui::Begin("Debug Info");
	ImGui::Checkbox("Show XZ plane grid", &showXZGrid);
	ImGui::Checkbox("Show world origin axis triad", &showAxisTriad);
	ImGui::Checkbox("Show camera target position", &showCameraTarget);
	ImGui::End();

	Vector3 pbrLightPosition = Vector3(0.0f, 4.0f, 2.0f);
	Vector3 pbrLightColor = Vector3(1.0f, 1.0f, 1.0f);

	Vector3 pbrMaterialDiffuse = Vector3(1.0f, 1.0f, 1.0f);
	Vector3 pbrMaterialRf0 = Vector3(0.015f, 0.015f, 0.015f);
	float pbrMaterialN = 64.0f;

	ImGui::Begin("Phong");
	ImGui::DragFloat3("Light position", &pbrLightPosition.x, 0.1f, -5.0f, 5.0f);
	ImGui::ColorEdit3("Light color", &pbrLightColor.x);
	ImGui::ColorEdit3("Material diffuse", &pbrMaterialDiffuse.x);
	ImGui::ColorEdit3("Material Rf0", &pbrMaterialRf0.x, 0.01f);
	ImGui::DragFloat("Material n", &pbrMaterialN, 0.5f, 1.0f, 1500.0f);
	ImGui::End();
}

void ModuleImGui::handleEditTransform(float* cameraView, float* cameraProjection, float* matrix)
{
	static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::ROTATE);
	static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);
	if (ImGui::IsKeyPressed(ImGuiKey_T))
		mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	if (ImGui::IsKeyPressed(ImGuiKey_E))
		mCurrentGizmoOperation = ImGuizmo::ROTATE;
	if (ImGui::IsKeyPressed(ImGuiKey_R))
		mCurrentGizmoOperation = ImGuizmo::SCALE;
	if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
		mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
		mCurrentGizmoOperation = ImGuizmo::ROTATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
		mCurrentGizmoOperation = ImGuizmo::SCALE;
	float matrixTranslation[3], matrixRotation[3], matrixScale[3];
	ImGuizmo::DecomposeMatrixToComponents(matrix, matrixTranslation, matrixRotation, matrixScale);
	ImGui::DragFloat3("Position", matrixTranslation, 0.1f, -100.0f, 100.0f);
	ImGui::DragFloat3("Rotation", matrixRotation, 0.1f, -100.0f, 100.0f);
	ImGui::DragFloat3("Scale", matrixScale, 0.1f, -100.0f, 100.0f);
	ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, matrix);

	if (mCurrentGizmoOperation != ImGuizmo::SCALE)
	{
		if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
			mCurrentGizmoMode = ImGuizmo::LOCAL;
		ImGui::SameLine();
		if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
			mCurrentGizmoMode = ImGuizmo::WORLD;
	}
	ImGuiIO& io = ImGui::GetIO();
	ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
	ImGuizmo::Manipulate(cameraView, cameraProjection, mCurrentGizmoOperation, mCurrentGizmoMode, matrix, NULL, NULL);
}

void ModuleImGui::showDebugGizmosWindow() {
	if (showXZGrid)
		dd::xzSquareGrid(-20.0f, 20.0f, 0.0f, 1.0f, dd::colors::LightGray);
	if (showAxisTriad) {
		// To avoid z-fighting between axis and grid. Axis lines always drawn on top
		Vector3 nudge = app->getModuleCamera()->GetTransform().position;
		nudge.Normalize();
		Matrix axisPos = Matrix::CreateTranslation(nudge * 0.001f);
		dd::axisTriad(ddConvert(axisPos), 0.05f, 0.5f);
	}
	if (showCameraTarget) {
		float cameraTargetColor[3] = { 1.0f, 0.0f, 0.0f };
		Vector3 cameraTarget = app->getModuleCamera()->getTarget();
		dd::sphere(&cameraTarget.x, cameraTargetColor, 0.025f);
	}
}