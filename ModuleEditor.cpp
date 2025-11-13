#include "Globals.h"
#include "ModuleEditor.h"
#include "ImGuiPass.h"
#include "ModuleD3D12.h"


bool ModuleEditor::postInit() {
	return true;
}

void ModuleEditor::preRender() {
	imGuiPass->startFrame();
}

void ModuleEditor::render() {
	ImGui::ShowDemoWindow();

	ID3D12GraphicsCommandList* currentBufferCommandList = app->getModuleD3D12()->getCurrentBufferCommandList().Get();
	imGuiPass->record(currentBufferCommandList, {});
}

void ModuleEditor::postRender() {};