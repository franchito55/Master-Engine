#pragma once
#include "Globals.h"
#include "Module.h"
#include "ImGuizmo.h"

#define FPS_PLOTTING_MAX 60

class ImGuiPass;
class ModuleD3D12;
class ModuleCameraEditor;

class ModuleImGui : public Module {
public:
	ModuleImGui(HWND _hWnd) : hWnd(_hWnd) {};
	~ModuleImGui() {};

	bool init() override;
	//void update() override;
	void preRender() override;
	void render() override;
	//void postRender() override;

	// Log to ImGui console
	void log(const char* t);

	ImVec2 getSceneRenderWindowSize() { return sceneRenderWindowSize; }
	
	bool getIsSceneRenderWindowHovered() { return sceneRenderWindowHovered; }

private:
	HWND hWnd;
	ModuleD3D12* moduleD3D12 = nullptr;
	ModuleCameraEditor* moduleCamera = nullptr;
	ImGuiPass* imGuiPass = nullptr;

	// FPS info window
	unsigned int fpsCount = 0;
	float frameTimes[FPS_PLOTTING_MAX] = {};
	float fps[FPS_PLOTTING_MAX] = {};
	unsigned int minFps = 99999;
	unsigned int maxFps = 0;

	// Show/Don't show debugging gizmos
	bool showXZGrid = true;
	bool showAxisTriad = true;
	bool showCameraTarget = false;
	bool showGeometryGizmo = true;

	// Console
	std::vector<std::string> consoleLog;
	bool scrollConsoleToBottom = true;

	ImVec2 sceneRenderWindowSize = { 400, 400 };
	ImVec2 sceneRenderWindowPos = { 0, 0 };
	ImVec2 sceneRenderWindowCursorPos = { 0, 0 };
	ImVec2 sceneRenderWindowImageRectMin = { 0, 0 };
	ImVec2 sceneRenderWindowImageRectMax = { 0, 0 };

	ImGuizmo::OPERATION mCurrentGizmoOperation = ImGuizmo::ROTATE;
	ImGuizmo::MODE mCurrentGizmoMode = ImGuizmo::WORLD;

	bool sceneRenderWindowHovered = false;

	Matrix modelMatrixBeforeGizmo = {};
	// Have to store euler angles separately in UI to avoid gimbal lock
	Vector3 uiRotationDeg = {};

	void showFpsInfoWindow();
	void showTextureInfoWindow();
	void showGeometryInfoWindow();
	void showLightingInfoWindow();
	void showMaterialInfoWindow();
	void showDebugGizmosWindow();
	void showConsoleWindow();
	void showCameraInfoWindow();
	void showSceneRenderWindow();

	bool compareVectors(float* v0, float* v1);
};