#pragma once
#include "Globals.h"
#include "Module.h"
#include "DebugDrawPass.h"
#include "structs/Transform.h"
#include "Mesh.h"
#include "GameObject.h"

class Application;
extern Application* app;

struct MvpCB
{
	Matrix mvp;
};

struct ModelMatrixCB
{
	Matrix modelMatrix;
};

struct NormalMatrixCB
{
	Matrix normalMatrix;
};

struct CameraCB
{
	Vector3 cameraPos; // 12B
	float _pad0; // Pad to 16 bytes (must do with cbuffers)
};

struct MaterialCB
{
	Vector3 materialRf0;
	float _padding;
	Vector3 materialDiffuse;
	float _padding2;
	float materialN;
};

struct LightCB
{
	Vector3 lightPos;
	float _pad1; // Padding to 16B
	Vector3 lightColor;
	float _pad2; // Padding to 16B
};

class ModuleAssignment2 : public Module {
public:
	ModuleAssignment2(HWND _hWnd);
	bool init() override;
	bool postInit() override;
	void update() override;
	void preRender() override;
	void render() override;
	void postRender() override;
	void createVertexBufferView(D3D12_VERTEX_BUFFER_VIEW* _vBV, GameObject& gO);
	void createIndexBufferView(D3D12_INDEX_BUFFER_VIEW* _iBV, GameObject& gO);
	Vector3* getObjectPosition() { return &gameObject->getTransform()->position; }

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

	GameObject* gameObject = nullptr;

	Matrix model;
	Matrix mvp;

	DebugDrawPass* debugDrawPass;

	ComPtr<ID3D12Resource> mvpCB = nullptr;
	MvpCB* mvpData = nullptr;

	ComPtr<ID3D12Resource> modelCB = nullptr;
	ModelMatrixCB* modelData = nullptr;

	ComPtr<ID3D12Resource> normalCB = nullptr;
	NormalMatrixCB* normalData = nullptr;

	ComPtr<ID3D12Resource> cameraCB = nullptr;
	CameraCB* cameraData = nullptr;

	ComPtr<ID3D12Resource> lightCB = nullptr;
	LightCB* lightData = nullptr;

	ComPtr<ID3D12Resource> materialCB = nullptr;
	MaterialCB* materialData = nullptr;


	Vector3 pbrLightPosition = Vector3(0.0f, 4.0f, 2.0f);
	Vector3 pbrLightColor = Vector3(1.0f, 1.0f, 1.0f);

	Vector3 pbrMaterialDiffuse = Vector3(1.0f, 1.0f, 1.0f);
	Vector3 pbrMaterialRf0 = Vector3(0.015f, 0.015f, 0.015f);
	float pbrMaterialN = 64.0f;


	bool showAxisTriad = true;
	bool showXZGrid = true;
	bool showCameraTarget = true;

	int currentTextureFiltering = 0;
	int currentTextureAddressingMode = 0;

	bool textureFilteringChanged = false;
	bool textureAddressingChanged = false;

	void buildRootSignature(ComPtr<ID3D12Device> device);
	void buildPSO(ComPtr<ID3D12Device> device);
	void initConstantBufferViews(ComPtr<ID3D12Device> device);

	D3D12_FILTER imGuiFilteringToDX12(unsigned int imGuiIndex);
	D3D12_TEXTURE_ADDRESS_MODE imGuiAddressingToDX12(unsigned int imGuiIndex);
	GameObject* createGameObjectFromGLTF(unsigned int meshIndex, unsigned int primitiveIndex);

	void log(const char* t);

	std::vector<std::string> consoleLog;
	bool scrollConsoleToBottom = true;
};