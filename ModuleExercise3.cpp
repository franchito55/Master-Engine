#include "Globals.h"
#include "ModuleExercise3.h"
#include "Application.h"
#include "ModuleD3D12.h"
#include "ReadData.h"
#include "ModuleBuffer.h"
#include "ImGuiPass.h"

extern Application* app;

ModuleExercise3::ModuleExercise3(HWND _hWnd) : hWnd(_hWnd) {};

bool ModuleExercise3::init(){
	ComPtr<ID3D12Device> device = app->getModuleD3D12()->getDevice();
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	CD3DX12_ROOT_PARAMETER rootSigDescParameters;
	rootSigDescParameters.InitAsConstants(sizeof(Matrix) / sizeof(UINT32), 0, 0, D3D12_SHADER_VISIBILITY_ALL);
	rootSigDesc.Init(0, &rootSigDescParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ComPtr<ID3DBlob> rootSigBlob;
	D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSigBlob, nullptr);
	HRESULT hr1 = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = rootSignature.Get();
	auto dataVS = DX::ReadData(L"Exercise2VS.cso");
	auto dataPS = DX::ReadData(L"Exercise2PS.cso");
	psoDesc.VS = { dataVS.data(), dataVS.size() };
	psoDesc.PS = { dataPS.data(), dataPS.size() };
	D3D12_INPUT_ELEMENT_DESC layoutDesc = {};
	layoutDesc.SemanticName = "MY_POS";
	layoutDesc.SemanticIndex = 0;
	layoutDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
	layoutDesc.InputSlot = 0;
	layoutDesc.AlignedByteOffset = 0;
	layoutDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	layoutDesc.InstanceDataStepRate = 0;
	D3D12_INPUT_ELEMENT_DESC layout[] = { layoutDesc };
	psoDesc.InputLayout = { layout, sizeof(layout) / sizeof(D3D12_INPUT_ELEMENT_DESC) };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.NumRenderTargets = 1;
	psoDesc.SampleDesc = { 1, 0 };
	psoDesc.SampleMask = 0xffffffff;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	HRESULT hr2 = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));

	app->getModuleBuffer()->createDefaultBuffer(vertexBuffer, 9 * sizeof(float));
	app->getModuleBuffer()->createUploadBuffer(stagingBuffer, 9 * sizeof(float));

	createVertexBufferView(&vBV);

	// Get a pointer to the resource in CPU (pData)
	BYTE* pData = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	HRESULT hr = stagingBuffer.Get()->Map(0, &readRange, reinterpret_cast<void**>(&pData));

	// Copy the data from the CPU array to the Resource
	memcpy(pData, vertices, sizeof(vertices));

	// Invalidates the pointer -> probably marks it as "used" ???
	stagingBuffer.Get()->Unmap(0, nullptr);

	// Init the camera matrices
	Matrix model = Matrix(
		transform.scale.x * transform.rotation.x, transform.scale.x * transform.rotation.y, transform.scale.x * transform.rotation.z, transform.position.x,
		transform.scale.y * transform.rotation.x, transform.scale.x * transform.rotation.y, transform.scale.x * transform.rotation.z, transform.position.y,
		transform.scale.z * transform.rotation.x, transform.scale.x * transform.rotation.y, transform.scale.x * transform.rotation.z, transform.position.z,
		0, 0, 0, 1
	).Transpose();

	Vector3 camForward = Vector3(camera.target - camera.position);
	camForward.Normalize();
	Vector3 camRight = camera.up.Cross(camForward);
	Vector3 camUp = camForward.Cross(camRight);
	Matrix view = Matrix(
		camRight.x, camRight.y, camRight.z, -camera.position.Dot(camRight),
		camUp.x, camUp.y, camUp.z, -camera.position.Dot(camUp),
		-camForward.x, -camForward.y, -camForward.z, camera.position.Dot(camForward),
		0, 0, 0, 1
	).Transpose();

	float projection00 = 1 / ((app->getWindowWidth() / app->getWindowHeight()) * tan(cameraFov / 2));
	float projection11 = 1 / (tan(cameraFov / 2));
	float projection22 = farPlane / (nearPlane - farPlane);
	float projection23 = (farPlane * nearPlane) / (nearPlane - farPlane);
	Matrix projection = Matrix(
		projection00, 0, 0, 0,
		0, projection11, 0, 0,
		0, 0, projection22, projection23,
		0, 0, -1, 0
	).Transpose();

	mvp = (model * view * projection).Transpose();

	ComPtr<ID3D12GraphicsCommandList> copyCommandList = app->getModuleD3D12()->getCopyCommandList();
	copyCommandList->CopyResource(vertexBuffer.Get(), stagingBuffer.Get());
	copyCommandList->SetGraphicsRoot32BitConstants(0, sizeof(Matrix) / sizeof(UINT32), &mvp, 0);
	copyCommandList->Close();
	ID3D12CommandList* lists[] = { copyCommandList.Get() };
	app->getModuleD3D12()->getCopyCommandQueue()->ExecuteCommandLists(1, lists);
	app->getModuleD3D12()->getCopyCommandQueue()->Signal(app->getModuleD3D12()->getFence().Get(), 500);
	app->getModuleD3D12()->WaitForFence(500);

	return true;
}

void ModuleExercise3::update() {
	Matrix model = Matrix(
		transform.scale.x * transform.rotation.x, transform.scale.x * transform.rotation.y, transform.scale.x * transform.rotation.z, transform.position.x,
		transform.scale.y * transform.rotation.x, transform.scale.x * transform.rotation.y, transform.scale.x * transform.rotation.z, transform.position.y,
		transform.scale.z * transform.rotation.x, transform.scale.x * transform.rotation.y, transform.scale.x * transform.rotation.z, transform.position.z,
		0, 0, 0, 1
	).Transpose();

	Vector3 camForward = Vector3(camera.target - camera.position);
	camForward.Normalize();
	Vector3 camRight = camera.up.Cross(camForward);
	Vector3 camUp = camForward.Cross(camRight);
	Matrix view = Matrix(
		camRight.x, camRight.y, camRight.z, -camera.position.Dot(camRight),
		camUp.x, camUp.y, camUp.z, -camera.position.Dot(camUp),
		-camForward.x, -camForward.y, -camForward.z, camera.position.Dot(camForward),
		0, 0, 0, 1
	).Transpose();

	float projection00 = 1 / ((app->getWindowWidth() / app->getWindowHeight()) * tan(cameraFov / 2));
	float projection11 = 1 / (tan(cameraFov / 2));
	float projection22 = farPlane / (nearPlane - farPlane);
	float projection23 = (farPlane * nearPlane) / (nearPlane - farPlane);
	Matrix projection = Matrix(
		projection00, 0, 0, 0,
		0, projection11, 0, 0,
		0, 0, projection22, projection23,
		0, 0, -1, 0
	).Transpose();

	mvp = model * view * projection;
}

void ModuleExercise3::preRender() {}

void ModuleExercise3::render() {
	ComPtr<ID3D12GraphicsCommandList> commandList = app->getModuleD3D12()->getCurrentBufferCommandList();

	commandList->SetPipelineState(pso.Get());

	commandList->OMSetRenderTargets(1, app->getModuleD3D12()->getCurrentRtvCpuDescriptorHandle(), FALSE, nullptr);
	float color[4] = { 0.2f, 0.2f, 0.2f, 1.0f};
	commandList->ClearRenderTargetView(*app->getModuleD3D12()->getCurrentRtvCpuDescriptorHandle(), color, 0, nullptr);

	commandList->SetGraphicsRootSignature(rootSignature.Get());
	commandList->IASetVertexBuffers(0, 1, &vBV);
	D3D12_VIEWPORT vp = { 0.0f, 0.0f, float(app->getWindowWidth()), float(app->getWindowHeight()), 0.0f, 1.0f };
	commandList->RSSetViewports(1, &vp);
	D3D12_RECT scissor = { 0, 0, app->getWindowWidth(), app->getWindowHeight() };
	commandList->RSSetScissorRects(1, &scissor);
	commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->DrawInstanced(3, 1, 0, 0);
}

void ModuleExercise3::postRender() {}

void ModuleExercise3::createVertexBufferView(D3D12_VERTEX_BUFFER_VIEW* vBV) {
	vBV->BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vBV->SizeInBytes = sizeof(vertices);
	vBV->StrideInBytes = 3 * sizeof(float);
}