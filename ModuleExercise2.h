#pragma once
#include "Globals.h"
#include "Module.h"

class ModuleExercise2 : public Module {
public:
	ModuleExercise2(HWND _hWnd);
	bool init() override;
	bool postInit() override;
	void preRender() override;
	void render() override;
	void postRender() override;
private:
	HWND hWnd;
	ComPtr<ID3D12PipelineState> pso;
	ComPtr<ID3D12RootSignature> rootSignature;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	D3D12_VERTEX_BUFFER_VIEW vBV;
};