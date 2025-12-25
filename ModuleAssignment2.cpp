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
#include "GameObject.h"
#include "Material.h"

ModuleAssignment2::ModuleAssignment2(HWND _hWnd) : hWnd(_hWnd) {}

bool ModuleAssignment2::init() {

	// The duck is HUGE for some reason (hundreds of units big)
	transform.scale = Vector3(0.01f, 0.01f, 0.01f);

	app->setModuleAssignment2(this);

	ComPtr<ID3D12Device> device = app->getModuleD3D12()->getDevice();

	// Read the vertex and index buffers and the texture from GLTF and pack them into a GameObject
	gameObject = createGameObjectFromGLTF(0, 0);

	unsigned int vertsInTopLeftQuadrant = 0;
	unsigned int vertsInTopRightQuadrant = 0;
	unsigned int vertsInBottomLeftQuadrant = 0;
	unsigned int vertsInBottomRightQuadrant = 0;
	unsigned int vertsOutsideOf01 = 0;
	for (unsigned int i = 0; i < gameObject->getMesh()->getNumVertices(); i++) {
		Mesh mesh = *gameObject->getMesh();
		char buffer[500];
		Vertex currentVertex = mesh.getVertices()[i];
		snprintf(buffer, sizeof(buffer), "Vertex %d:\n  Position: (%f, %f, %f)\n  Normal: (%f, %f, %f)\n TexCoord: (%f, %f)\n", i, 
			currentVertex.position.x, currentVertex.position.y, currentVertex.position.z,
			currentVertex.normal.x, currentVertex.normal.y, currentVertex.normal.z,
			currentVertex.texCoord.x, currentVertex.texCoord.y);
		log(buffer);
		
		if (currentVertex.texCoord.x < 0.5 && currentVertex.texCoord.y < 0.5)
			vertsInTopLeftQuadrant++;
		else if (currentVertex.texCoord.x > 0.614 && currentVertex.texCoord.y < 0.465)
			vertsInTopRightQuadrant++;
		else if (currentVertex.texCoord.x < 0.5 && currentVertex.texCoord.y > 0.5)
			vertsInBottomLeftQuadrant++;
		else if (currentVertex.texCoord.x > 0.5 && currentVertex.texCoord.y > 0.5)
			vertsInBottomRightQuadrant++;

		else if (currentVertex.texCoord.x < 0 || currentVertex.texCoord.y > 1 || currentVertex.texCoord.y < 0 || currentVertex.texCoord.y > 1)
			vertsOutsideOf01++;
	}
	char buffer[500];
	snprintf(buffer, sizeof(buffer), "Vertices in top left quadrant: %d", vertsInTopLeftQuadrant);
	log(buffer);
	snprintf(buffer, sizeof(buffer), "Vertices in top right quadrant: %d", vertsInTopRightQuadrant);
	log(buffer);
	snprintf(buffer, sizeof(buffer), "Vertices in bottom left quadrant: %d", vertsInBottomLeftQuadrant);
	log(buffer);
	snprintf(buffer, sizeof(buffer), "Vertices in bottom right quadrant: %d", vertsInBottomRightQuadrant);
	log(buffer);
	snprintf(buffer, sizeof(buffer), "Vertices outside of [0, 1]: %d", vertsOutsideOf01);
	log(buffer);

	/*for (unsigned int i = 0; i < gameObject->getMesh()->getNumIndices(); i+=3) {
		Mesh mesh = *gameObject->getMesh();
		char buffer[500];
		snprintf(buffer, sizeof(buffer), "Triangle %d: %d, %d, %d,\n", i, mesh.getIndices()[i], mesh.getIndices()[i+1], mesh.getIndices()[i+2]);
		log(buffer);
	}*/

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

	model = Matrix::CreateScale(transform.scale) * Matrix::CreateTranslation(transform.position);
	mvp = (model * app->getModuleCamera()->GetViewMatrix() * app->getModuleCamera()->GetProjectionMatrix()).Transpose();

	ComPtr<ID3D12GraphicsCommandList> commandList = app->getModuleD3D12()->getCurrentBufferCommandList();

	commandList->SetPipelineState(pso.Get());

	commandList->OMSetRenderTargets(1, app->getModuleD3D12()->getCurrentRtvCpuDescriptorHandle(), FALSE, app->getModuleD3D12()->getDSVCPUDescriptorHandle());
	float backgroundColor[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
	commandList->ClearRenderTargetView(*app->getModuleD3D12()->getCurrentRtvCpuDescriptorHandle(), backgroundColor, 0, nullptr);

	commandList->SetGraphicsRootSignature(rootSignature.Get());
	commandList->SetGraphicsRoot32BitConstants(0, sizeof(Matrix) / sizeof(UINT32), &mvp, 0);
	commandList->IASetIndexBuffer(&iBV);
	commandList->IASetVertexBuffers(0, 1, &vBV);
	D3D12_VIEWPORT vp = { 0.0f, 0.0f, float(app->getWindowWidth()), float(app->getWindowHeight()), 0.0f, 1.0f };
	commandList->RSSetViewports(1, &vp);
	D3D12_RECT scissor = { 0, 0, app->getWindowWidth(), app->getWindowHeight() };
	commandList->RSSetScissorRects(1, &scissor);
	commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D12DescriptorHeap* srvHeap[1] = {app->getModuleD3D12()->getShaderVisibleDescriptorHeap().Get()};
	commandList->SetDescriptorHeaps(1, srvHeap);
	
	commandList->SetGraphicsRootDescriptorTable(1, app->getModuleD3D12()->getShaderVisibleDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());

	commandList->DrawIndexedInstanced(gameObject->getMesh()->getNumIndices(), 1, 0, 0, 0);


	ImGui::Begin("Texture info");

	int prevFilteringMode = currentTextureFiltering;
	const char* filteringModes[] = { "LINEAR", "POINT" };
	ImGui::Combo("Filtering mode", &currentTextureFiltering, filteringModes, IM_ARRAYSIZE(filteringModes));
	if (currentTextureFiltering != prevFilteringMode) // Set this flag to change texture filtering mode next frame
		textureFilteringChanged = true;

	int prevAddressingMode = currentTextureAddressingMode;
	const char* addressingModes[] = { "WRAP", "CLAMP" };
	ImGui::Combo("Addressing mode", &currentTextureAddressingMode, addressingModes, IM_ARRAYSIZE(addressingModes));
	if (currentTextureAddressingMode != prevAddressingMode) // Set this flag to change texture addressing mode next frame
		textureAddressingChanged = true;

	ImGui::End();

	// Quad info window
	ImGui::Begin("Geometry");
	ImGui::DragFloat3("Quad position", &transform.position.x, 0.1f, -100.0f, 100.0f);
	ImGui::End();

	ImGui::Begin("Debug Info");
	ImGui::Checkbox("Show XZ plane grid", &showXZGrid);
	ImGui::Checkbox("Show world origin axis triad", &showAxisTriad);
	ImGui::Checkbox("Show camera target position", &showCameraTarget);
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

	CD3DX12_ROOT_PARAMETER rootParameters[2] = {};

	rootParameters[0].InitAsConstants(sizeof(Matrix) / sizeof(UINT32), 0, 0, D3D12_SHADER_VISIBILITY_ALL);

	CD3DX12_DESCRIPTOR_RANGE tableRange;
	tableRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	rootParameters[1].InitAsDescriptorTable(1, &tableRange, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_STATIC_SAMPLER_DESC sampler;
	sampler.Init(0, 
		imGuiFilteringToDX12(currentTextureFiltering), // Linear filtering
		imGuiAddressingToDX12(currentTextureAddressingMode), // Addressing mode
		imGuiAddressingToDX12(currentTextureAddressingMode),
		imGuiAddressingToDX12(currentTextureAddressingMode));

	rootSigDesc.Init(2, rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
	// Get a pointer to the resource in CPU (pData)
	BYTE* pData = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	HRESULT hr = stagingVertexBuffer.Get()->Map(0, &readRange, reinterpret_cast<void**>(&pData));

	// Copy the data from the CPU array to the Resource
	memcpy(pData, gO->getMesh()->getVertices(), gO->getMesh()->getNumVertices() * sizeof(Vertex));

	// Invalidates the pointer -> probably marks it as "used" ???
	stagingVertexBuffer.Get()->Unmap(0, nullptr);


	// Index buffer
	app->getModuleBuffer()->createDefaultBuffer(gpuIndexBuffer, gO->getMesh()->getNumIndices() * sizeof(unsigned short));
	app->getModuleBuffer()->createUploadBuffer(stagingIndexBuffer, gO->getMesh()->getNumIndices() * sizeof(unsigned short));
	createIndexBufferView(&iBV, *gO);

	// Copy index buffer
	// Get a pointer to the resource in CPU (pData)
	hr = stagingIndexBuffer.Get()->Map(0, &readRange, reinterpret_cast<void**>(&pData));

	// Copy the data from the CPU array to the Resource
	memcpy(pData, gO->getMesh()->getIndices(), gO->getMesh()->getNumIndices() * sizeof(unsigned short));

	// Invalidates the pointer -> probably marks it as "used" ???
	stagingIndexBuffer.Get()->Unmap(0, nullptr);

	// Load texture from GLTF
	Utils::loadTextureIntoGameObjectGLTF(tinyGLTFModel, meshIndex, primitiveIndex, stagingTextureBuffer, gpuTextureBuffer);

	// Init the camera matrices
	Matrix model = Matrix::CreateScale(transform.scale) * Matrix::CreateTranslation(transform.position);

	mvp = (model * app->getModuleCamera()->GetViewMatrix() * app->getModuleCamera()->GetProjectionMatrix()).Transpose();

	return gO;
}

void ModuleAssignment2::log(const char* t) {
	consoleLog.emplace_back(t);
}