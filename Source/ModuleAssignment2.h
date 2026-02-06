#pragma once
#include "Globals.h"
#include "Module.h"
#include "DebugDrawPass.h"
#include "structs/Transform.h"
#include "Mesh.h"
#include "GameObject.h"

class CameraComponent;

class Application;
extern Application* app;

struct CameraCB
{
	Vector3 cameraPos; // 12B
	float _pad0; // Pad to 16 bytes (must do with cbuffers)
};

struct MaterialCB
{
	Vector3 materialFresnel0;
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
	float lightIntensity;
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

	int* getCurrentTextureFilteringMode() { return &currentTextureFiltering; } // Return * for ImGui editing
	void setTextureFilteringChanged(bool _textureFilteringChanged) { textureFilteringChanged = _textureFilteringChanged; }
	int* getCurrentTextureAddressingMode() { return &currentTextureAddressingMode; } // Return * for ImGui editing
	void setTextureAddressingChanged(bool _textureAddressingChanged) { textureAddressingChanged = _textureAddressingChanged; }

	Vector3* getLightPosition() { return &pbrLightPosition; }
	Vector3* getLightColor() { return &pbrLightColor; }
	float* getLightIntensity() { return &pbrLightIntensity; }
	Vector3* getMaterialDiffuse() { return &pbrMaterialDiffuse; }
	Vector3* getMaterialFresnel0() { return &pbrMaterialFresnel0; }
	float* getMaterialN() { return &pbrMaterialN; }

	// Frustum culling, multiple objects in scene
	void renderGameObject(GameObject* gameObject, ComPtr<ID3D12GraphicsCommandList> commandList);

	std::vector<GameObject*> getGameObjects() const { return gameObjects; }

	inline UINT align256(UINT size);

	CameraComponent* getCameraComponent() const { return cameraComponent; }

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


	DebugDrawPass* debugDrawPass;

	ComPtr<ID3D12Resource> cameraCB = nullptr;
	CameraCB* cameraData = nullptr;

	ComPtr<ID3D12Resource> lightCB = nullptr;
	LightCB* lightData = nullptr;

	ComPtr<ID3D12Resource> materialCB = nullptr;
	MaterialCB* materialData = nullptr;


	Vector3 pbrLightPosition = Vector3(0.0f, 4.0f, 2.0f);
	Vector3 pbrLightColor = Vector3(1.0f, 1.0f, 1.0f);
	float pbrLightIntensity = 3.5f;

	Vector3 pbrMaterialDiffuse = Vector3(1.0f, 1.0f, 1.0f);
	Vector3 pbrMaterialFresnel0 = Vector3(0.015f, 0.015f, 0.015f);
	float pbrMaterialN = 64.0f;


	int currentTextureFiltering = 0;
	int currentTextureAddressingMode = 0;

	bool textureFilteringChanged = false;
	bool textureAddressingChanged = false;

	float timePassed = 0.0f;

	void buildRootSignature();
	void buildPSO();
	void initConstantBufferViews(GameObject* gameObject);

	D3D12_FILTER imGuiFilteringToDX12(unsigned int imGuiIndex);
	D3D12_TEXTURE_ADDRESS_MODE imGuiAddressingToDX12(unsigned int imGuiIndex);
	GameObject* createGameObjectFromGLTF(const std::string fileName, unsigned int meshIndex, unsigned int primitiveIndex);

	void addGridOfDucks();

	std::vector<GameObject*> gameObjects;

	CameraComponent* cameraComponent;
};