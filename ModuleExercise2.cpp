#include "Globals.h"
#include "ModuleExercise2.h"
#include "Application.h"
#include "ModuleD3D12.h"
#include "ReadData.h"
#include "ModuleBuffer.h"

extern Application* app;

ModuleExercise2::ModuleExercise2(HWND _hWnd) : hWnd(_hWnd) {};

bool ModuleExercise2::init(){ return true; }

bool ModuleExercise2::postInit(){ 
	ComPtr<ID3D12Device> device = app->getModuleD3D12()->getDevice();
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ComPtr<ID3DBlob> rootSigBlob;
	D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSigBlob, nullptr);
	ComPtr<ID3D12RootSignature> rootSignature;
	device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = rootSignature.Get();
	auto dataVS = DX::ReadData(L"Exercise2VS.hlsl");
	auto dataPS = DX::ReadData(L"Exercise2PS.hlsl");
	psoDesc.VS = { dataVS.data(), dataVS.size() };
	psoDesc.PS = { dataPS.data(), dataPS.size() };
	D3D12_INPUT_ELEMENT_DESC layoutDesc = {};
	layoutDesc.SemanticName = "MY_POS";
	layoutDesc.SemanticIndex = 0;
	layoutDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
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
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));

	createVertexBufferView(&vBV);

	app->getModuleBuffer()->createDefaultBuffer(stagingBuffer, 9 * sizeof(float));
	app->getModuleBuffer()->createUploadBuffer(vertexBuffer, 9 * sizeof(float));

	// Get a pointer to the resource in CPU (pData)
	BYTE* pData = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	HRESULT hr = stagingBuffer.Get()->Map(0, &readRange, reinterpret_cast<void**>(&pData));

	// Copy the data from the CPU array to the Resource
	memcpy(pData, vertices, sizeof(vertices));

	// Invalidates the pointer -> probably marks it as "used" ???
	stagingBuffer.Get()->Unmap(0, nullptr);

	return true;
}

void ModuleExercise2::preRender() {}

void ModuleExercise2::render() {
	ComPtr<ID3D12GraphicsCommandList> commandList = app->getModuleD3D12()->getCurrentBufferCommandList();
	commandList->SetGraphicsRootSignature(rootSignature.Get());
	commandList->IASetVertexBuffers(0, 1, &vBV);
	D3D12_VIEWPORT vp = { 0.0f, 0.0f, float(app->getWindowWidth()), float(app->getWindowHeight()), 0.0f, 1.0f };
	commandList->RSSetViewports(1, &vp);
	D3D12_RECT scissor = { 0, 0, app->getWindowWidth(), app->getWindowHeight() };
	commandList->RSSetScissorRects(1, &scissor);
	commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(3, 1, 0, 0);
}

void ModuleExercise2::postRender() {}

void ModuleExercise2::createVertexBufferView(D3D12_VERTEX_BUFFER_VIEW* vBV) {
	vBV->BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vBV->SizeInBytes = 9 * sizeof(float);
	vBV->StrideInBytes = 3 * sizeof(float);
}