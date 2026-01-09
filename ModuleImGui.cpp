#include "Globals.h"
#include "ModuleImGui.h"
#include "ImGuiPass.h"
#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleAssignment2.h"

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