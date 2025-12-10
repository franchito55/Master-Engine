#pragma once
#include "Globals.h"
#include "Module.h"
#include "ModuleCameraEditor.h"
#include "DebugDrawPass.h"

#define FLOATS_PER_VERTEX 5

class Application;
extern Application* app;

struct Vertex {
	Vector3 position;
	Vector2 uv;
};

class ModuleAssignment1 : public Module {
public:
	ModuleAssignment1(HWND _hWnd);
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
	ComPtr<ID3D12Resource> stagingVertexBuffer = nullptr;
	ComPtr<ID3D12Resource> gpuVertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	D3D12_VERTEX_BUFFER_VIEW vBV = {};
	ComPtr<ID3D12Resource> stagingTextureBuffer = nullptr;
	ComPtr<ID3D12Resource> gpuTextureBuffer = nullptr;

	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvGpuHandle, textureSamplerGpuHandle;

	Vertex vertices[6] = {
		{ Vector3(-1.0f, 1.0f, 0.0f), Vector2(-0.25f, -0.25f) },
		{ Vector3(-1.0f, -1.0f, 0.0f), Vector2(-0.25f, 1.25f) },
		{ Vector3(1.0f, -1.0f, 0.0f), Vector2(1.25f, 1.25f) },
		{ Vector3(-1.0f, 1.0f, 0.0f), Vector2(-0.25f, -0.25f)},
		{ Vector3(1.0f, -1.0f, 0.0f), Vector2(1.25f, 1.25f) },
		{ Vector3(1.0f, 1.0f, 0.0f), Vector2(1.25f, -0.25f) }
	};

	Transform transform = {};

	Matrix model;
	Matrix mvp;

	DebugDrawPass* debugDrawPass;

	bool showAxisTriad = true;
	bool showXZGrid = true;
};