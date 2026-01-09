#pragma once
#include "Globals.h"
#include "Module.h"

#define FPS_PLOTTING_MAX 60

class ImGuiPass;
class ModuleD3D12;

class ModuleImGui : public Module {
public:
	ModuleImGui(HWND _hWnd) : hWnd(_hWnd) {};
	~ModuleImGui() {};

	bool init() override;
	//void update() override;
	void preRender() override;
	void render() override;
	//void postRender() override;


private:
	HWND hWnd;
	ModuleD3D12* moduleD3D12 = nullptr;
	ImGuiPass* imGuiPass = nullptr;

	unsigned int fpsCount = 0;
	float frameTimes[FPS_PLOTTING_MAX] = {};
	float fps[FPS_PLOTTING_MAX] = {};
	unsigned int minFps = 99999;
	unsigned int maxFps = 0;

	void showFpsInfoWindow();
	void showTextureInfoWindow();
	void showGeometryInfoWindow();
	void showLightingInfoWindow();
};