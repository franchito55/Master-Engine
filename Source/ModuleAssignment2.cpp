#include "Globals.h"
#include <iostream>
#define _USE_MATH_DEFINES
#include "math.h"
#include "ModuleAssignment2.h"
#include "Application.h"
#include "ModuleD3D12.h"
#include "ReadData.h"
#include "ModuleResources.h"
#include "ImGuiPass.h"
#include "ModuleCameraEditor.h"
#include "GLTFLoader.h"
#include "Material.h"
#include "ImGuizmo.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleNonShaderDescriptors.h"
#include "ModuleImGui.h"

#define PI 3.14159265359

#if defined(_DEBUG)
#define ASSETS_RELATIVE_PATH "../"
#else
#define ASSETS_RELATIVE_PATH "/resources/"
#endif

ModuleAssignment2::ModuleAssignment2(HWND _hWnd) : hWnd(_hWnd) {}

bool ModuleAssignment2::init() {

	// Read the vertex and index buffers and the texture from GLTF and pack them into a GameObject
	// The duck is HUGE for some reason (hundreds of units big)

	gameObjects.push_back(createGameObjectFromGLTF("Duck.gltf", 0, 0));
	gameObjects.at(0)->getTransform().position = Vector3(0.0f, 0.0f, 0.0f);
	gameObjects.at(0)->getTransform().scale = Vector3(0.01f, 0.01f, 0.01f);
	gameObjects.push_back(createGameObjectFromGLTF("Duck.gltf", 0, 0));
	gameObjects.at(1)->getTransform().position = Vector3(-2.0f, 0.0f, 0.0f);
	gameObjects.at(1)->getTransform().scale = Vector3(0.01f, 0.01f, 0.01f);
	gameObjects.push_back(createGameObjectFromGLTF("Duck.gltf", 0, 0));
	gameObjects.at(2)->getTransform().position = Vector3(2.0f, 0.0f, 0.0f);
	gameObjects.at(2)->getTransform().scale = Vector3(0.01f, 0.01f, 0.01f);

	for (unsigned int i = 0; i < gameObjects.size(); i++) {
		initConstantBufferViews(gameObjects.at(i));
	}

	// Create and map once cameraCB (this would normally be on the Camera --> move to Camera)
	app->getModuleResources().createUploadBuffer(cameraCB, sizeof(CameraCB));
	cameraCB->Map(0, nullptr, reinterpret_cast<void**>(&cameraData));

	// Create and map once lightCB (this would normally be on the Light --> move to Light)
	app->getModuleResources().createUploadBuffer(lightCB, sizeof(LightCB));
	lightCB->Map(0, nullptr, reinterpret_cast<void**>(&lightData));

	// Create and map once materialCB
	app->getModuleResources().createUploadBuffer(materialCB, sizeof(MaterialCB));
	materialCB->Map(0, nullptr, reinterpret_cast<void**>(&materialData));

	buildRootSignature();

	buildPSO();

	ComPtr<ID3D12Device> device = app->getModuleD3D12().getDevice();
	// Init DebugDrawPass (for drawing axis and stuff)
	ComPtr<ID3D12Device4> d4;
	device->QueryInterface(IID_PPV_ARGS(&d4));
	debugDrawPass = new DebugDrawPass(d4.Get(), app->getModuleD3D12().getRenderCommandQueue().Get(), false);

	return true;
}

bool ModuleAssignment2::postInit() {
	ComPtr<ID3D12GraphicsCommandList> commandList = app->getModuleD3D12().getCurrentBufferCommandList();
	// We must transition the GPU index buffer to D3D12_RESOURCE_STATE_INDEX_BUFFER
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(gpuIndexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE);
	commandList->ResourceBarrier(1, &barrier);
	return true;
}

void ModuleAssignment2::update() {}

void ModuleAssignment2::preRender() {
	if (textureFilteringChanged || textureAddressingChanged) {
		app->getModuleD3D12().WaitForAllFences();
		buildRootSignature();
		buildPSO();
		textureFilteringChanged = false;
		textureAddressingChanged = false;
	}
}

void ModuleAssignment2::render() {

	ComPtr<ID3D12GraphicsCommandList> commandList = app->getModuleD3D12().getCurrentBufferCommandList();

	commandList->SetPipelineState(pso.Get());

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		app->getModuleD3D12().getSceneRenderTexture().Get(),                  // the texture resource
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	commandList->ResourceBarrier(1, &barrier);

	//unsigned int rtvIndexInRTVHeap = app->getModuleD3D12().getCurrentRTVIndexInRTVHeap();
	//D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuDescriptorHandle = app->getModuleNonShaderDescriptors().getCPUHandleFromRTVHeap(rtvIndexInRTVHeap);
	unsigned int dsvIndexInDSVHeap = app->getModuleD3D12().getDSVIndexInDSVHeap();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuDescriptorHandle = app->getModuleNonShaderDescriptors().getCPUHandleFromDSVHeap(dsvIndexInDSVHeap);
	// TODO: change the render target to the texture
	unsigned int sceneRTVIndexInHeap = app->getModuleD3D12().getSceneRTVIndexInHeap();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuDescriptorHandle = app->getModuleNonShaderDescriptors().getCPUHandleFromRTVHeap(sceneRTVIndexInHeap);
	commandList->OMSetRenderTargets(1, &rtvCpuDescriptorHandle, FALSE, &dsvCpuDescriptorHandle);
	float backgroundColor[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
	commandList->ClearRenderTargetView(rtvCpuDescriptorHandle, backgroundColor, 0, nullptr);

	commandList->SetGraphicsRootSignature(rootSignature.Get());

	cameraData->cameraPos = app->getModuleCameraEditor().getTransform().position;
	commandList->SetGraphicsRootConstantBufferView(4, cameraCB->GetGPUVirtualAddress());

	materialData->materialN = pbrMaterialN;
	materialData->materialFresnel0 = pbrMaterialFresnel0;
	materialData->materialDiffuse = pbrMaterialDiffuse;
	commandList->SetGraphicsRootConstantBufferView(5, materialCB->GetGPUVirtualAddress());

	lightData->lightPos = pbrLightPosition;
	lightData->lightColor = pbrLightColor;
	lightData->lightIntensity = pbrLightIntensity;
	commandList->SetGraphicsRootConstantBufferView(6, lightCB->GetGPUVirtualAddress());

	// Set viewport & scissor
	D3D12_VIEWPORT vp = { 0.0f, 0.0f, float(app->getSceneRenderWindowWidth()), float(app->getSceneRenderWindowHeight()), 0.0f, 1.0f };
	commandList->RSSetViewports(1, &vp);
	D3D12_RECT scissor = { 0, 0, app->getSceneRenderWindowWidth(), app->getSceneRenderWindowHeight() };
	commandList->RSSetScissorRects(1, &scissor);

	ID3D12DescriptorHeap* srvHeap = app->getModuleShaderDescriptors().getDescriptorHeap().Get();
	ID3D12DescriptorHeap* srvHeaps[1] = {srvHeap};
	commandList->SetDescriptorHeaps(1, srvHeaps);
	
	commandList->SetGraphicsRootDescriptorTable(3, srvHeap->GetGPUDescriptorHandleForHeapStart());

	for (unsigned int i = 0; i < gameObjects.size(); i++) {
		renderGameObject(gameObjects.at(i), commandList);
	}

	// Draw debug info last so it's on top
	debugDrawPass->record(commandList.Get(), app->getSceneRenderWindowWidth(), app->getSceneRenderWindowHeight(), app->getModuleCameraEditor().getViewMatrix(), app->getModuleCameraEditor().getProjectionMatrix());

	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		app->getModuleD3D12().getSceneRenderTexture().Get(),                  // the texture resource
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	commandList->ResourceBarrier(1, &barrier);
}

void ModuleAssignment2::renderGameObject(GameObject* gameObject, ComPtr<ID3D12GraphicsCommandList> commandList) {
	gameObject->setModelMatrix(Matrix::CreateScale(gameObject->getTransform().scale) *
		Matrix::CreateFromQuaternion(gameObject->getTransform().rotation) *
		Matrix::CreateTranslation(gameObject->getTransform().position));
	gameObject->setMvpMatrix(gameObject->getModelMatrix() * app->getModuleCameraEditor().getViewMatrix() * app->getModuleCameraEditor().getProjectionMatrix().Transpose());

	// Set cbuffers
	gameObject->getMvpData()->mvp = gameObject->getMvpMatrix();
	commandList->SetGraphicsRootConstantBufferView(0, gameObject->getMvpCB()->GetGPUVirtualAddress());

	// Need to transpose model for row-major in GPU
	gameObject->getModelMatrixData()->modelMatrix = gameObject->getModelMatrix().Transpose();
	commandList->SetGraphicsRootConstantBufferView(1, gameObject->getModelCB()->GetGPUVirtualAddress());

	// Normal matrix = Transpose of inverse of model matrix
	Matrix invTransModel = gameObject->getModelMatrix().Invert().Transpose();
	gameObject->getNormalData()->normalMatrix = {
		invTransModel._11, invTransModel._12, invTransModel._13,
		invTransModel._21, invTransModel._22, invTransModel._23,
		invTransModel._31, invTransModel._32, invTransModel._33,
	};
	commandList->SetGraphicsRootConstantBufferView(2, gameObject->getNormalCB()->GetGPUVirtualAddress());

	commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set current index and vertex buffers
	commandList->IASetIndexBuffer(&iBV);
	commandList->IASetVertexBuffers(0, 1, &vBV);

	commandList->DrawIndexedInstanced(gameObject->getMesh().getNumIndices(), 1, 0, 0, 0);
}

void ModuleAssignment2::postRender() {}

void ModuleAssignment2::createVertexBufferView(D3D12_VERTEX_BUFFER_VIEW* vBV, GameObject& gO) {
	vBV->BufferLocation = gpuVertexBuffer->GetGPUVirtualAddress();
	vBV->SizeInBytes = gO.getMesh().getNumVertices() * sizeof(Vertex);
	vBV->StrideInBytes = sizeof(Vertex);
}

void ModuleAssignment2::createIndexBufferView(D3D12_INDEX_BUFFER_VIEW* iBV, GameObject& gO) {
	iBV->BufferLocation = gpuIndexBuffer->GetGPUVirtualAddress();
	iBV->SizeInBytes = gO.getMesh().getNumIndices() * sizeof(unsigned short);
	iBV->Format = DXGI_FORMAT_R16_UINT;
}

D3D12_FILTER ModuleAssignment2::imGuiFilteringToDX12(unsigned int imGuiIndex) {
	switch (imGuiIndex) {
	case 0:
		return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	case 1:
		return D3D12_FILTER_MIN_MAG_MIP_POINT;
	}
}

D3D12_TEXTURE_ADDRESS_MODE ModuleAssignment2::imGuiAddressingToDX12(unsigned int imGuiIndex) {
	switch (imGuiIndex) {
	case 0:
		return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	case 1:
		return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	}
}

void ModuleAssignment2::buildRootSignature() {
	ComPtr<ID3D12Device> device = app->getModuleD3D12().getDevice();
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc = {};

	CD3DX12_ROOT_PARAMETER rootParameters[7] = {};

	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_DESCRIPTOR_RANGE tableRange;
	tableRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	rootParameters[3].InitAsDescriptorTable(1, &tableRange, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_STATIC_SAMPLER_DESC sampler;
	sampler.Init(0, 
		imGuiFilteringToDX12(currentTextureFiltering), // Linear filtering
		imGuiAddressingToDX12(currentTextureAddressingMode), // Addressing mode
		imGuiAddressingToDX12(currentTextureAddressingMode),
		imGuiAddressingToDX12(currentTextureAddressingMode));

	rootParameters[4].InitAsConstantBufferView(3, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[5].InitAsConstantBufferView(4, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[6].InitAsConstantBufferView(5, 0, D3D12_SHADER_VISIBILITY_PIXEL);

	rootSigDesc.Init(7, rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> rootSigBlob;
	D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSigBlob, nullptr);
	device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
}

void ModuleAssignment2::buildPSO() {
	ComPtr<ID3D12Device> device = app->getModuleD3D12().getDevice();
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = rootSignature.Get();
	auto dataVS = DX::ReadData(L"VertexShader.cso");
	auto dataPS = DX::ReadData(L"PixelShader.cso");
	psoDesc.VS = { dataVS.data(), dataVS.size() };
	psoDesc.PS = { dataPS.data(), dataPS.size() };
	D3D12_INPUT_ELEMENT_DESC layoutDescPos = {};
	layoutDescPos.SemanticName = "MY_POS";
	layoutDescPos.SemanticIndex = 0;
	layoutDescPos.Format = DXGI_FORMAT_R32G32B32_FLOAT;
	layoutDescPos.InputSlot = 0;
	layoutDescPos.AlignedByteOffset = 0;
	layoutDescPos.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	layoutDescPos.InstanceDataStepRate = 0;
	D3D12_INPUT_ELEMENT_DESC layoutDescNormals = {};
	layoutDescNormals.SemanticName = "NORMAL";
	layoutDescNormals.SemanticIndex = 0;
	layoutDescNormals.Format = DXGI_FORMAT_R32G32B32_FLOAT;
	layoutDescNormals.InputSlot = 0;
	layoutDescNormals.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	layoutDescNormals.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	layoutDescNormals.InstanceDataStepRate = 0;
	D3D12_INPUT_ELEMENT_DESC layoutDescTexCoord = {};
	layoutDescTexCoord.SemanticName = "TEXCOORD";
	layoutDescTexCoord.SemanticIndex = 0;
	layoutDescTexCoord.Format = DXGI_FORMAT_R32G32_FLOAT;
	layoutDescTexCoord.InputSlot = 0;
	layoutDescTexCoord.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	layoutDescTexCoord.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	layoutDescTexCoord.InstanceDataStepRate = 0;
	D3D12_INPUT_ELEMENT_DESC layout[] = { layoutDescPos, layoutDescNormals, layoutDescTexCoord };
	psoDesc.InputLayout = { layout, sizeof(layout) / sizeof(D3D12_INPUT_ELEMENT_DESC) };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.NumRenderTargets = 1;
	psoDesc.SampleDesc = { 1, 0 };
	psoDesc.SampleMask = 0xffffffff;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
}

GameObject* ModuleAssignment2::createGameObjectFromGLTF(const std::string fileName, unsigned int meshIndex, unsigned int primitiveIndex) {
	GameObject* gO = new GameObject();

	// =================== Load model using GLTF ===================
	tinygltf::TinyGLTF tinyGLTF;
	tinygltf::Model tinyGLTFModel;
	std::string error, warning;
	const std::string filePath = ASSETS_RELATIVE_PATH + fileName;
	tinyGLTF.LoadASCIIFromFile(&tinyGLTFModel, &error, &warning, filePath);
	if (error != "") {
		MessageBoxA(hWnd, ("Cannot find file: " + filePath).c_str(), "Error loading GLTF file", 0);
		exit(500);
	}

	// Load vertices from GLTF
	GLTFLoader::loadMeshIntoGameObjectGLTF(tinyGLTFModel, meshIndex, primitiveIndex, gO);

	// Vertex buffer
	app->getModuleResources().createDefaultBufferWithData(gpuVertexBuffer, gO->getMesh().getVertices(), gO->getMesh().getNumVertices() * sizeof(Vertex));
	createVertexBufferView(&vBV, *gO);


	// Index buffer
	app->getModuleResources().createDefaultBufferWithData(gpuIndexBuffer, gO->getMesh().getIndices(), gO->getMesh().getNumIndices() * sizeof(unsigned short));
	createIndexBufferView(&iBV, *gO);


	// Load texture from GLTF
	GLTFLoader::loadTextureIntoGameObjectGLTF(tinyGLTFModel, meshIndex, primitiveIndex, stagingTextureBuffer, gpuTextureBuffer);

	// Init the camera matrices
	Matrix model = Matrix::CreateScale(gO->getTransform().scale) * Matrix::CreateTranslation(gO->getTransform().position);

	gO->setMvpMatrix((model * app->getModuleCameraEditor().getViewMatrix() * app->getModuleCameraEditor().getProjectionMatrix()).Transpose());

	return gO;
}

void ModuleAssignment2::initConstantBufferViews(GameObject* gameObject) {
	ComPtr<ID3D12Device> device = app->getModuleD3D12().getDevice();
	// Create and map once MvpCB
	app->getModuleResources().createDefaultBuffer(sizeof(MvpCB));
	ComPtr<ID3D12Resource> mvpCB = gameObject->getMvpCB();
	app->getModuleResources().createUploadBuffer(mvpCB, (unsigned int)sizeof(MvpCB));
	gameObject->getMvpCB()->Map(0, nullptr, reinterpret_cast<void**>(gameObject->getMvpData()));

	// Create and map once ModelCB
	ComPtr<ID3D12Resource> modelCB = gameObject->getModelCB();
	app->getModuleResources().createUploadBuffer(modelCB, sizeof(ModelMatrixCB));
	gameObject->getModelCB()->Map(0, nullptr, reinterpret_cast<void**>(gameObject->getModelMatrixData()));

	// Create and map once normalCB
	ComPtr<ID3D12Resource> normalCB = gameObject->getModelCB();
	app->getModuleResources().createUploadBuffer(normalCB, sizeof(NormalMatrixCB));
	gameObject->getNormalCB()->Map(0, nullptr, reinterpret_cast<void**>(gameObject->getNormalData()));
}