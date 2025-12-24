#pragma once
#include "Globals.h"
#include "Module.h"
#include "DebugDrawPass.h"
#include "structs/Transform.h"
#include "Mesh.h"

#define FLOATS_PER_VERTEX 8 // Position (3), Texture coordinates (2), Normal (3)

struct Vertex;
class Application;
extern Application* app;

class ModuleAssignment2 : public Module {
public:
	ModuleAssignment2(HWND _hWnd);
	bool init() override;
	bool postInit() override;
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
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	D3D12_VERTEX_BUFFER_VIEW vBV = {};
	D3D12_INDEX_BUFFER_VIEW iBV = {};
	ComPtr<ID3D12Resource> stagingTextureBuffer = nullptr;
	ComPtr<ID3D12Resource> gpuTextureBuffer = nullptr;

	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvGpuHandle, textureSamplerGpuHandle;

	/*Vertex vertices[6] = {
		{ Vector3(-1.0f, 1.0f, 0.0f), Vector2(-0.25f, -0.25f) },
		{ Vector3(-1.0f, -1.0f, 0.0f), Vector2(-0.25f, 1.25f) },
		{ Vector3(1.0f, -1.0f, 0.0f), Vector2(1.25f, 1.25f) },
		{ Vector3(-1.0f, 1.0f, 0.0f), Vector2(-0.25f, -0.25f)},
		{ Vector3(1.0f, -1.0f, 0.0f), Vector2(1.25f, 1.25f) },
		{ Vector3(1.0f, 1.0f, 0.0f), Vector2(1.25f, -0.25f) }
	};*/

	Mesh mesh;

	Transform transform = {};

	Matrix model;
	Matrix mvp;

	DebugDrawPass* debugDrawPass;

	bool showAxisTriad = true;
	bool showXZGrid = true;
	bool showCameraTarget = true;

	int currentTextureFiltering = 0;
	int currentTextureAddressingMode = 0;

	bool textureFilteringChanged = false;
	bool textureAddressingChanged = false;

	void buildRootSignature(ComPtr<ID3D12Device> device);
	void buildPSO(ComPtr<ID3D12Device> device);
	D3D12_FILTER imGuiFilteringToDX12(unsigned int imGuiIndex);
	D3D12_TEXTURE_ADDRESS_MODE imGuiAddressingToDX12(unsigned int imGuiIndex);
	void readMeshFromGLTF();

	void log(const char* t);

	std::vector<std::string> consoleLog;
	bool scrollConsoleToBottom = true;
};