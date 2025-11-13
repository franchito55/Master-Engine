#pragma once
#include "Module.h"

class ImGuiPass;

class ModuleEditor : public Module {
public:
	ModuleEditor(HWND _hWnd) : hWnd(_hWnd) {};
	~ModuleEditor() { delete imGuiPass; }

	bool postInit();
	void preRender() override;
	void render() override;
	void postRender() override;
	
private:
	HWND hWnd;
	ImGuiPass* imGuiPass;
};