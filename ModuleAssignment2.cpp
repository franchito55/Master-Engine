#include "Globals.h"
#include <iostream>
#define _USE_MATH_DEFINES
#include "math.h"
#include "ModuleAssignment2.h"
#include "Application.h"
#include "ModuleD3D12.h"
#include "ReadData.h"
#include "ModuleBuffer.h"
#include "ImGuiPass.h"
#include "ModuleCameraEditor.h"
#include "Utils.h"
#include "Material.h"
#include "ImGuizmo.h"

ModuleAssignment2::ModuleAssignment2(HWND _hWnd) : hWnd(_hWnd) {}

bool ModuleAssignment2::init() {

	app->setModuleAssignment2(this);

	ComPtr<ID3D12Device> device = app->getModuleD3D12()->getDevice();

	// Read the vertex and index buffers and the texture from GLTF and pack them into a GameObject
	gameObject = createGameObjectFromGLTF(0, 0);
	// The duck is HUGE for some reason (hundreds of units big)
	gameObject->getTransform()->scale = Vector3(0.01f, 0.01f, 0.01f);


	initConstantBufferViews(device);

	buildRootSignature(device);

	buildPSO(device);
	

	// Init DebugDrawPass (for drawing axis and stuff)
	ComPtr<ID3D12Device4> d4;
	device->QueryInterface(IID_PPV_ARGS(&d4));
	debugDrawPass = new DebugDrawPass(d4.Get(), app->getModuleD3D12()->getRenderCommandQueue().Get(), false);


	// Copy data (vertex and index buffers) to GPU buffers
	ComPtr<ID3D12GraphicsCommandList> copyCommandList = app->getModuleD3D12()->getCopyCommandList();
	copyCommandList->CopyResource(gpuVertexBuffer.Get(), stagingVertexBuffer.Get());
	copyCommandList->CopyResource(gpuIndexBuffer.Get(), stagingIndexBuffer.Get());
	copyCommandList->Close();

	ID3D12CommandList* lists[] = { copyCommandList.Get() };
	app->getModuleD3D12()->getCopyCommandQueue()->ExecuteCommandLists(1, lists);
	app->getModuleD3D12()->getCopyCommandQueue()->Signal(app->getModuleD3D12()->getFence().Get(), 500);
	app->getModuleD3D12()->WaitForFence(500);


	model = Matrix::CreateScale(gameObject->getTransform()->scale) * Matrix::CreateTranslation(gameObject->getTransform()->position);

	return true;
}

bool ModuleAssignment2::postInit() {
	ComPtr<ID3D12GraphicsCommandList> commandList = app->getModuleD3D12()->getCurrentBufferCommandList();
	// We must transition the GPU index buffer to D3D12_RESOURCE_STATE_INDEX_BUFFER
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(gpuIndexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE);
	commandList->ResourceBarrier(1, &barrier);
	return true;
}

void ModuleAssignment2::update() {}

void ModuleAssignment2::preRender() {
	if (textureFilteringChanged || textureAddressingChanged) {
		app->getModuleD3D12()->WaitForAllFences();
		buildRootSignature(app->getModuleD3D12()->getDevice());
		buildPSO(app->getModuleD3D12()->getDevice());
		textureFilteringChanged = false;
		textureAddressingChanged = false;
	}
}

void ModuleAssignment2::render() {
	mvp = (model * app->getModuleCamera()->GetViewMatrix() * app->getModuleCamera()->GetProjectionMatrix()).Transpose();

	ComPtr<ID3D12GraphicsCommandList> commandList = app->getModuleD3D12()->getCurrentBufferCommandList();

	commandList->SetPipelineState(pso.Get());

	commandList->OMSetRenderTargets(1, app->getModuleD3D12()->getCurrentRtvCpuDescriptorHandle(), FALSE, app->getModuleD3D12()->getDSVCPUDescriptorHandle());
	float backgroundColor[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
	commandList->ClearRenderTargetView(*app->getModuleD3D12()->getCurrentRtvCpuDescriptorHandle(), backgroundColor, 0, nullptr);

	commandList->SetGraphicsRootSignature(rootSignature.Get());

	// Set cbuffers
	mvpData->mvp = mvp;
	commandList->SetGraphicsRootConstantBufferView(0, mvpCB->GetGPUVirtualAddress());

	// Need to transpose model for row-major in GPU
	modelData->modelMatrix = model.Transpose();
	commandList->SetGraphicsRootConstantBufferView(1, modelCB->GetGPUVirtualAddress());

	// Normal matrix = Transpose of inverse of model matrix
	Matrix invTransModel = model.Invert().Transpose();
	normalData->normalMatrix = {
		invTransModel._11, invTransModel._12, invTransModel._13,
		invTransModel._21, invTransModel._22, invTransModel._23,
		invTransModel._31, invTransModel._32, invTransModel._33,
	};
	commandList->SetGraphicsRootConstantBufferView(2, normalCB->GetGPUVirtualAddress());

	cameraData->cameraPos = app->getModuleCamera()->GetTransform().position;
	commandList->SetGraphicsRootConstantBufferView(4, cameraCB->GetGPUVirtualAddress());

	materialData->materialN = pbrMaterialN;
	materialData->materialRf0 = pbrMaterialRf0;
	materialData->materialDiffuse = pbrMaterialDiffuse;
	commandList->SetGraphicsRootConstantBufferView(5, materialCB->GetGPUVirtualAddress());

	lightData->lightPos = pbrLightPosition;
	lightData->lightColor = pbrLightColor;
	commandList->SetGraphicsRootConstantBufferView(6, lightCB->GetGPUVirtualAddress());

	// Set current index and vertex buffers
	commandList->IASetIndexBuffer(&iBV);
	commandList->IASetVertexBuffers(0, 1, &vBV);

	// Set viewport & scissor (whole screen)
	D3D12_VIEWPORT vp = { 0.0f, 0.0f, float(app->getWindowWidth()), float(app->getWindowHeight()), 0.0f, 1.0f };
	commandList->RSSetViewports(1, &vp);
	D3D12_RECT scissor = { 0, 0, app->getWindowWidth(), app->getWindowHeight() };
	commandList->RSSetScissorRects(1, &scissor);

	commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D12DescriptorHeap* srvHeap[1] = {app->getModuleD3D12()->getShaderVisibleDescriptorHeap().Get()};
	commandList->SetDescriptorHeaps(1, srvHeap);
	
	commandList->SetGraphicsRootDescriptorTable(3, app->getModuleD3D12()->getShaderVisibleDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());

	commandList->DrawIndexedInstanced(gameObject->getMesh()->getNumIndices(), 1, 0, 0, 0);


	// Quad info window
	ImGui::Begin("Geometry");
	Matrix cameraViewMatrix = app->getModuleCamera()->GetViewMatrix();
	Matrix cameraProjMatrix = app->getModuleCamera()->GetProjectionMatrix();
	ImGuizmo::BeginFrame();
	handleEditTransform(&cameraViewMatrix._11, &cameraProjMatrix._11, &model._11);
	ImGui::End();

	ImGui::Begin("Debug Info");
	ImGui::Checkbox("Show XZ plane grid", &showXZGrid);
	ImGui::Checkbox("Show world origin axis triad", &showAxisTriad);
	ImGui::Checkbox("Show camera target position", &showCameraTarget);
	ImGui::End();

	ImGui::Begin("Phong");
	ImGui::DragFloat3("Light position", &pbrLightPosition.x, 0.1f, -5.0f, 5.0f);
	ImGui::ColorEdit3("Light color", &pbrLightColor.x);
	ImGui::ColorEdit3("Material diffuse", &pbrMaterialDiffuse.x);
	ImGui::ColorEdit3("Material Rf0", &pbrMaterialRf0.x, 0.01f);
	ImGui::DragFloat("Material n", &pbrMaterialN, 0.5f, 1.0f, 1500.0f);
	ImGui::End();

	dd::clear();

	if (showXZGrid)
		dd::xzSquareGrid(-20.0f, 20.0f, 0.0f, 1.0f, dd::colors::LightGray);
	if (showAxisTriad) {
		// To avoid z-fighting between axis and grid. Axis lines always drawn on top
		Vector3 nudge = app->getModuleCamera()->GetTransform().position;
		nudge.Normalize();
		Matrix axisPos = Matrix::CreateTranslation(nudge * 0.001f);
		dd::axisTriad(ddConvert(axisPos), 0.05f, 0.5f);
	}
	if (showCameraTarget) {
		float cameraTargetColor[3] = { 1.0f, 0.0f, 0.0f };
		Vector3 cameraTarget = app->getModuleCamera()->getTarget();
		dd::sphere(&cameraTarget.x, cameraTargetColor, 0.025f);
	}

	// Output console
	ImGui::Begin("Console");
	if (ImGui::Button("Clear")) {
		consoleLog.clear();
	}
	ImGui::SameLine();
	if (ImGui::Button("Copy")) {
		ImGui::LogToClipboard();
		for (const auto& line : consoleLog) ImGui::TextUnformatted(line.c_str());
		ImGui::LogFinish();
	}
	ImGui::Separator();
	ImGui::BeginChild("Log", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
	for (const auto& line : consoleLog) {
		ImGui::TextUnformatted(line.c_str());
	}

	if (scrollConsoleToBottom) {
		ImGui::SetScrollHereY(1.0f);
		scrollConsoleToBottom = false;
	}
	ImGui::EndChild();
	ImGui::End();

	debugDrawPass->record(commandList.Get(), app->getWindowWidth(), app->getWindowHeight(), app->getModuleCamera()->GetViewMatrix(), app->getModuleCamera()->GetProjectionMatrix());
}

void ModuleAssignment2::postRender() {}

void ModuleAssignment2::createVertexBufferView(D3D12_VERTEX_BUFFER_VIEW* vBV, GameObject& gO) {
	vBV->BufferLocation = gpuVertexBuffer->GetGPUVirtualAddress();
	vBV->SizeInBytes = gO.getMesh()->getNumVertices() * sizeof(Vertex);
	vBV->StrideInBytes = sizeof(Vertex);
}

void ModuleAssignment2::createIndexBufferView(D3D12_INDEX_BUFFER_VIEW* iBV, GameObject& gO) {
	iBV->BufferLocation = gpuIndexBuffer->GetGPUVirtualAddress();
	iBV->SizeInBytes = gO.getMesh()->getNumIndices() * sizeof(unsigned short);
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

void ModuleAssignment2::buildRootSignature(ComPtr<ID3D12Device> device) {
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

void ModuleAssignment2::buildPSO(ComPtr<ID3D12Device> device) {
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

GameObject* ModuleAssignment2::createGameObjectFromGLTF(unsigned int meshIndex, unsigned int primitiveIndex) {
	GameObject* gO = new GameObject();

	// =================== Load model using GLTF ===================
	tinygltf::TinyGLTF tinyGLTF;
	tinygltf::Model tinyGLTFModel;
	std::string error, warning;
	tinyGLTF.LoadASCIIFromFile(&tinyGLTFModel, &error, &warning, "../Duck.gltf");
	if (error != "") {
		MessageBoxA(hWnd, "Cannot find file", "Error loading GLTF file", 0);
	}

	// Load vertices from GLTF
	Utils::loadMeshIntoGameObjectGLTF(tinyGLTFModel, meshIndex, primitiveIndex, gO);

	// Vertex buffer
	app->getModuleBuffer()->createDefaultBuffer(gpuVertexBuffer, gO->getMesh()->getNumVertices() * sizeof(Vertex));
	app->getModuleBuffer()->createUploadBuffer(stagingVertexBuffer, gO->getMesh()->getNumVertices() * sizeof(Vertex));
	createVertexBufferView(&vBV, *gO);

	// Copy vertex buffer
	ModuleBuffer::copyDataToBuffer(stagingVertexBuffer, gO->getMesh()->getVertices(), gO->getMesh()->getNumVertices() * sizeof(Vertex));


	// Index buffer
	app->getModuleBuffer()->createDefaultBuffer(gpuIndexBuffer, gO->getMesh()->getNumIndices() * sizeof(unsigned short));
	app->getModuleBuffer()->createUploadBuffer(stagingIndexBuffer, gO->getMesh()->getNumIndices() * sizeof(unsigned short));
	createIndexBufferView(&iBV, *gO);

	// Copy index buffer
	ModuleBuffer::copyDataToBuffer(stagingIndexBuffer, gO->getMesh()->getIndices(), gO->getMesh()->getNumIndices() * sizeof(unsigned short));


	// Load texture from GLTF
	Utils::loadTextureIntoGameObjectGLTF(tinyGLTFModel, meshIndex, primitiveIndex, stagingTextureBuffer, gpuTextureBuffer);

	// Init the camera matrices
	Matrix model = Matrix::CreateScale(gO->getTransform()->scale) * Matrix::CreateTranslation(gO->getTransform()->position);

	mvp = (model * app->getModuleCamera()->GetViewMatrix() * app->getModuleCamera()->GetProjectionMatrix()).Transpose();

	return gO;
}

void ModuleAssignment2::initConstantBufferViews(ComPtr<ID3D12Device> device) {
	// Create and map once MvpCB
	app->getModuleBuffer()->createUploadBuffer(mvpCB, sizeof(MvpCB));
	mvpCB->Map(0, nullptr, reinterpret_cast<void**>(&mvpData));

	// Create and map once ModelCB
	app->getModuleBuffer()->createUploadBuffer(modelCB, sizeof(ModelMatrixCB));
	modelCB->Map(0, nullptr, reinterpret_cast<void**>(&modelData));

	// Create and map once normalCB
	app->getModuleBuffer()->createUploadBuffer(normalCB, sizeof(NormalMatrixCB));
	normalCB->Map(0, nullptr, reinterpret_cast<void**>(&normalData));

	// Create and map once cameraCB (this would normally be on the Camera --> move to Camera)
	app->getModuleBuffer()->createUploadBuffer(cameraCB, sizeof(CameraCB));
	cameraCB->Map(0, nullptr, reinterpret_cast<void**>(&cameraData));

	// Create and map once MvpCB (this would normally be on the Light --> move to Light)
	app->getModuleBuffer()->createUploadBuffer(lightCB, sizeof(LightCB));
	lightCB->Map(0, nullptr, reinterpret_cast<void**>(&lightData));

	// Create and map once MvpCB
	app->getModuleBuffer()->createUploadBuffer(materialCB, sizeof(MaterialCB));
	materialCB->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
}

void ModuleAssignment2::log(const char* t) {
	consoleLog.emplace_back(t);
}

void ModuleAssignment2::handleEditTransform(float* cameraView, float* cameraProjection, float* matrix)
{
	static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::ROTATE);
	static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);
	if (ImGui::IsKeyPressed(ImGuiKey_T))
		mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	if (ImGui::IsKeyPressed(ImGuiKey_E))
		mCurrentGizmoOperation = ImGuizmo::ROTATE;
	if (ImGui::IsKeyPressed(ImGuiKey_R))
		mCurrentGizmoOperation = ImGuizmo::SCALE;
	if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
		mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
		mCurrentGizmoOperation = ImGuizmo::ROTATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
		mCurrentGizmoOperation = ImGuizmo::SCALE;
	float matrixTranslation[3], matrixRotation[3], matrixScale[3];
	ImGuizmo::DecomposeMatrixToComponents(matrix, matrixTranslation, matrixRotation, matrixScale);
	ImGui::DragFloat3("Position", matrixTranslation, 0.1f, -100.0f, 100.0f);
	ImGui::DragFloat3("Rotation", matrixRotation, 0.1f, -100.0f, 100.0f);
	ImGui::DragFloat3("Scale", matrixScale, 0.1f, -100.0f, 100.0f);
	ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, matrix);

	if (mCurrentGizmoOperation != ImGuizmo::SCALE)
	{
		if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
			mCurrentGizmoMode = ImGuizmo::LOCAL;
		ImGui::SameLine();
		if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
			mCurrentGizmoMode = ImGuizmo::WORLD;
	}
	ImGuiIO& io = ImGui::GetIO();
	ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
	ImGuizmo::Manipulate(cameraView, cameraProjection, mCurrentGizmoOperation, mCurrentGizmoMode, matrix, NULL, NULL);
}