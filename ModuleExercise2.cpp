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
	ComPtr<ID3D12Device> device = app->getDevice();
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
}

void ModuleExercise2::preRender() {}

void ModuleExercise2::render() {
	D3D12_VERTEX_BUFFER_VIEW vBV;
	vBV.BufferLocation = app->getModuleBuffer()->getVertexBuffer()->GetGPUVirtualAddress();
	vBV.SizeInBytes = 9 * sizeof(float);
	vBV.StrideInBytes = 3 * sizeof(float);
	app->getDevice()->CreateVerte
}