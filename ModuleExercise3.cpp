#include "Globals.h"
#define _USE_MATH_DEFINES
#include "ModuleExercise3.h"
#include "Application.h"
#include "ModuleD3D12.h"
#include "ReadData.h"
#include "ModuleBuffer.h"
#include "ImGuiPass.h"
#include "DebugDrawPass.h"
#include "math.h"
#include "ModuleCameraEditor.h"

extern Application* app;

ModuleExercise3::ModuleExercise3(HWND _hWnd) : hWnd(_hWnd) {};

bool ModuleExercise3::init(){
	ComPtr<ID3D12Device> device = app->getModuleD3D12()->getDevice();
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	CD3DX12_ROOT_PARAMETER rootSigDescParameters;
	rootSigDescParameters.InitAsConstants(sizeof(Matrix) / sizeof(UINT32), 0, 0, D3D12_SHADER_VISIBILITY_ALL);
	rootSigDesc.Init(1, &rootSigDescParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
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
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	HRESULT hr2 = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));

	app->getModuleBuffer()->createDefaultBuffer(gpuVertexBuffer, sizeof(vertices));
	app->getModuleBuffer()->createUploadBuffer(stagingVertexBuffer, sizeof(vertices));

	app->getModuleBuffer()->createDefaultBuffer(gpuIndexBuffer, sizeof(indices));
	app->getModuleBuffer()->createUploadBuffer(stagingIndexBuffer, sizeof(indices));

	createVertexBufferView(&vBV);
	createIndexBufferView(&iBV);

	// ============ Init vertex and index buffers ============
	// Get a pointer to the resource in CPU (pData)
	BYTE* pDataVertex = nullptr;
	CD3DX12_RANGE readRangeVertex(0, 0);
	stagingVertexBuffer.Get()->Map(0, &readRangeVertex, reinterpret_cast<void**>(&pDataVertex));

	// Copy the data from the CPU array to the Resource
	memcpy(pDataVertex, vertices, sizeof(vertices));

	// Invalidates the pointer -> probably marks it as "used" ???
	stagingVertexBuffer.Get()->Unmap(0, nullptr);

	// Get a pointer to the resource in CPU (pData)
	BYTE* pDataIndex = nullptr;
	CD3DX12_RANGE readRangeIndex(0, 0);
	stagingIndexBuffer.Get()->Map(0, &readRangeIndex, reinterpret_cast<void**>(&pDataIndex));

	// Copy the data from the CPU array to the Resource
	memcpy(pDataIndex, indices, sizeof(indices));

	// Invalidates the pointer -> probably marks it as "used" ???
	stagingIndexBuffer.Get()->Unmap(0, nullptr);


	// ============ Init the camera matrices ============
	Matrix model = Matrix::CreateScale(transform.scale) * Matrix::CreateTranslation(transform.position);

	mvp = (model * app->getModuleCamera()->GetViewMatrix() * app->getModuleCamera()->GetProjectionMatrix()).Transpose();


	// ============ Copy data (vertex and index buffers) to GPU buffers ============
	ComPtr<ID3D12GraphicsCommandList> copyCommandList = app->getModuleD3D12()->getCopyCommandList();
	copyCommandList->CopyResource(gpuVertexBuffer.Get(), stagingVertexBuffer.Get());
	copyCommandList->CopyResource(gpuIndexBuffer.Get(), stagingIndexBuffer.Get());
	copyCommandList->Close();
	ID3D12CommandList* lists[] = { copyCommandList.Get() };
	app->getModuleD3D12()->getCopyCommandQueue()->ExecuteCommandLists(1, lists);
	app->getModuleD3D12()->getCopyCommandQueue()->Signal(app->getModuleD3D12()->getFence().Get(), 500);
	app->getModuleD3D12()->WaitForFence(500);


	// ============ Init DebugDrawPass (for drawing axis and stuff) ============
	ComPtr<ID3D12Device4> d4;
	device->QueryInterface(IID_PPV_ARGS(&d4));
	debugDrawPass = new DebugDrawPass(d4.Get(), app->getModuleD3D12()->getRenderCommandQueue().Get());
	return true;
}

void ModuleExercise3::update() {}

void ModuleExercise3::preRender() {}

void ModuleExercise3::render() {

	model = Matrix::CreateScale(transform.scale) * Matrix::CreateTranslation(transform.position);
	mvp = (model * app->getModuleCamera()->GetViewMatrix() * app->getModuleCamera()->GetProjectionMatrix()).Transpose();

	ComPtr<ID3D12GraphicsCommandList> commandList = app->getModuleD3D12()->getCurrentBufferCommandList();

	commandList->SetPipelineState(pso.Get());

	commandList->OMSetRenderTargets(1, app->getModuleD3D12()->getCurrentRtvCpuDescriptorHandle(), FALSE, app->getModuleD3D12()->getDSVCPUDescriptorHandle());
	float color[4] = { 0.2f, 0.2f, 0.2f, 1.0f};
	commandList->ClearRenderTargetView(*app->getModuleD3D12()->getCurrentRtvCpuDescriptorHandle(), color, 0, nullptr);

	commandList->SetGraphicsRootSignature(rootSignature.Get());
	commandList->SetGraphicsRoot32BitConstants(0, sizeof(Matrix) / sizeof(UINT32), &mvp, 0);
	commandList->IASetVertexBuffers(0, 1, &vBV);
	commandList->IASetIndexBuffer(&iBV);
	D3D12_VIEWPORT vp = { 0.0f, 0.0f, float(app->getWindowWidth()), float(app->getWindowHeight()), 0.0f, 1.0f };
	commandList->RSSetViewports(1, &vp);
	D3D12_RECT scissor = { 0, 0, app->getWindowWidth(), app->getWindowHeight() };
	commandList->RSSetScissorRects(1, &scissor);
	commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->DrawIndexedInstanced(sizeof(indices) / sizeof(float), 1, 0, 0, 0);

	ModuleCameraEditor* camera = app->getModuleCamera();

	float cameraEye[3] = { camera->GetTransform().position.x, camera->GetTransform().position.x,camera->GetTransform().position.x };
	// Triangle position window
	ImGui::Begin("Triangle position");
	ImGui::DragFloat3("Triangle position", &transform.position.x, 0.1f, -100.0f, 100.0f);
	ImGui::End();

	// DebugDrawPass's record() sets the view and projection matrices AT THAT MOMENT, so you actually need to call it BEFORE
	// calling things like sphere() to avoid a mismatch between frame's informations. If you call sphere() before record(),
	// the sphere will be drawn using last frame's View and Projection, since they haven't been updated yet
	debugDrawPass->record(commandList.Get(), app->getWindowWidth(), app->getWindowHeight(), app->getModuleCamera()->GetViewMatrix(), app->getModuleCamera()->GetProjectionMatrix());
	dd::xzSquareGrid(-20.0f, 20.0f, 0.0f, 1.0f, dd::colors::LightGray);
	dd::axisTriad(ddConvert(Matrix::Identity), 0.05f, 0.5f);
	float cameraTargetColor[3] = { 1.0f, 0.0f, 0.0f };
	Vector3 cameraTarget = app->getModuleCamera()->getTarget();
	dd::sphere(&cameraTarget.x, cameraTargetColor, 0.05f);
}

void ModuleExercise3::postRender() {}

void ModuleExercise3::createVertexBufferView(D3D12_VERTEX_BUFFER_VIEW* vBV) {
	vBV->BufferLocation = gpuVertexBuffer->GetGPUVirtualAddress();
	vBV->SizeInBytes = sizeof(vertices);
	vBV->StrideInBytes = 3 * sizeof(float);
}

void ModuleExercise3::createIndexBufferView(D3D12_INDEX_BUFFER_VIEW* iBV) {
	iBV->BufferLocation = gpuIndexBuffer->GetGPUVirtualAddress();
	iBV->SizeInBytes = sizeof(indices);
	iBV->Format = DXGI_FORMAT_R32_UINT;
}