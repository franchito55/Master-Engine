#pragma once
#include "Globals.h"
#include "Module.h"

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

	// Console
	std::vector<std::string> consoleLog;
	bool scrollConsoleToBottom = true;


	void showFpsInfoWindow();
	void showTextureInfoWindow();
	void showGeometryInfoWindow();
	void showLightingInfoWindow();
	void showDebugGizmosWindow();
	void showConsoleWindow();
	void showCameraInfoWindow();

	void handleEditTransform(float* viewMatrix, float* projectionMatrix, float* modelMatrix);
};