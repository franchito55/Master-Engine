#include "Globals.h"
#include "ModuleEditor.h"
#include "ImGuiPass.h"
#include "ModuleD3D12.h"


bool ModuleEditor::postInit(ModuleD3D12* _moduleD3D12) {
	moduleD3D12 = _moduleD3D12;
	imGuiPass = new ImGuiPass(moduleD3D12->getDevice().Get(), hWnd, {0}, {0});
	return true;
}

void ModuleEditor::preRender() {
	imGuiPass->startFrame();
}

void ModuleEditor::render() {
	ImGui::ShowDemoWindow();

	ID3D12GraphicsCommandList* currentBufferCommandList = moduleD3D12->getCurrentBufferCommandList().Get();
	imGuiPass->record(currentBufferCommandList, {});
}

void ModuleEditor::postRender() {};