#pragma once
#include "Module.h"

class ModuleD3D12;
class ImGuiPass;

class ModuleEditor : public Module {
public:
	ModuleEditor(HWND _hWnd) : hWnd(_hWnd) {};
	~ModuleEditor() { delete imGuiPass; }

	bool postInit(ModuleD3D12* _moduleD3D12);
	void preRender() override;
	void render() override;
	void postRender() override;
	
private:
	HWND hWnd;
	ImGuiPass* imGuiPass;
	ModuleD3D12* moduleD3D12;
};