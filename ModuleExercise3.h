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
	void createIndexBufferView(D3D12_INDEX_BUFFER_VIEW* _iBV);
	Vector3 getTrianglePosition() { return transform.position; }
private:
	HWND hWnd;
	ComPtr<ID3D12PipelineState> pso = nullptr;
	ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	ComPtr<ID3D12Resource> stagingVertexBuffer = nullptr;
	ComPtr<ID3D12Resource> gpuVertexBuffer = nullptr;
	ComPtr<ID3D12Resource> stagingIndexBuffer = nullptr;
	ComPtr<ID3D12Resource> gpuIndexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vBV;
	D3D12_INDEX_BUFFER_VIEW iBV;

	//float vertices[9] = { 0.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f };

	// Cube version:
	float vertices[24] = {
		// Top face
		-0.5f, 0.5f, -0.5f, 
		-0.5f, 0.5f, 0.5f, 
		0.5f, 0.5f, 0.5f,
		0.5f, 0.5f, -0.5f,

		// Bottom face
		-0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f, 0.5f,
		0.5f, -0.5f, 0.5f,
		0.5f, -0.5f, -0.5f
	};

	unsigned int indices[36] = {
		// Top face
		0, 1, 2,
		0, 2, 3,

		// Bottom face
		5, 4, 7,
		5, 7, 6,

		// Right face
		2, 6, 7,
		2, 7, 3,

		// Left face
		0, 4, 5,
		0, 5, 1,

		// Front face
		1, 5, 6,
		1, 6, 2,

		// Back face
		3, 7, 4,
		3, 4, 0
	};

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