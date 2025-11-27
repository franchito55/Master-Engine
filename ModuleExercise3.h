#pragma once
#include "Globals.h"
#include "Module.h"
#include "ModuleCameraEditor.h"

class DebugDrawPass;

class ModuleExercise3 : public Module {
public:
	ModuleExercise3(HWND _hWnd);
	bool init() override;
	void update() override;
	void preRender() override;
	void render() override;
	void postRender() override;
	void createVertexBufferView(D3D12_VERTEX_BUFFER_VIEW* _vBV);
	Vector3 getTrianglePosition() { return transform.position; }
private:
	HWND hWnd;
	ComPtr<ID3D12PipelineState> pso = nullptr;
	ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	ComPtr<ID3D12Resource> stagingBuffer = nullptr;
	ComPtr<ID3D12Resource> vertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	D3D12_VERTEX_BUFFER_VIEW vBV;

	float vertices[9] = { 0.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f };

	Transform transform = {};

	float trianglePos[3] = { 0.0f, 0.0f, 0.0f };
	Matrix model;
	Matrix mvp;
	float cameraFov = 90;
	float nearPlane = 0.02f;
	float farPlane = 30.0f;

	DebugDrawPass* debugDrawPass;

	float cameraMoveSpeed = 1.0f;
};