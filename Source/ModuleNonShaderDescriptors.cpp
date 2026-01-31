#include "Globals.h"
#include "ModuleNonShaderDescriptors.h"
#include "Application.h"
#include "ModuleD3D12.h"

#define DESCRIPTOR_NUMBER 1000;

extern Application* app;

bool ModuleNonShaderDescriptors::init() {
	device = app->getModuleD3D12().getDevice();
	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	// ============ Init RTV descriptor heap (non-shader visible) ============ 
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	// Type = Render Target View
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	// How many RTVs (2 -> 1 back buffer, 1 front buffer)
	descriptorHeapDesc.NumDescriptors = DESCRIPTOR_NUMBER;
	// Shader-visible ?
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	// Flags for multi-adapter. Which adapter this descriptor is for
	descriptorHeapDesc.NodeMask = 0;
	device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&rtvHeap));
	rtvHeap->SetName(L"RTV Heap");

	// ============ Init DSV descriptor heap ============ 
	D3D12_DESCRIPTOR_HEAP_DESC dsbDescriptorHeap = {};
	// Type = Depth/Stencil View
	dsbDescriptorHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	// 1 depth buffer
	dsbDescriptorHeap.NumDescriptors = 1;
	// Non-shader-visible
	dsbDescriptorHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	// Flags for multi-adapter. Which adapter this descriptor is for
	dsbDescriptorHeap.NodeMask = 0;
	device->CreateDescriptorHeap(&dsbDescriptorHeap, IID_PPV_ARGS(&dsvHeap));
	dsvHeap->SetName(L"DSV Heap");

	return true;
}

unsigned int ModuleNonShaderDescriptors::createRTV(ID3D12Resource* resource) {
	unsigned int index = rtvHeapNextFreeIndex++;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle = getCPUHandleFromRTVHeap(index);
	device->CreateRenderTargetView(resource, nullptr, cpuDescriptorHandle);

	return index;
}

unsigned int ModuleNonShaderDescriptors::createRTV(ID3D12Resource* resource, D3D12_RENDER_TARGET_VIEW_DESC& desc) {
	unsigned int index = rtvHeapNextFreeIndex++;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle = getCPUHandleFromRTVHeap(index);
	device->CreateRenderTargetView(resource, &desc, cpuDescriptorHandle);

	return index;
}

unsigned int ModuleNonShaderDescriptors::createDSV(ID3D12Resource* resource) {
	CD3DX12_HEAP_PROPERTIES dsbHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC dsbHeapDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, app->getSceneRenderWindowWidth(), app->getSceneRenderWindowHeight(), 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;
	device->CreateCommittedResource(&dsbHeapProps, D3D12_HEAP_FLAG_NONE, &dsbHeapDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&resource));
	unsigned int index = dsvHeapNextFreeIndex++;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle = getCPUHandleFromDSVHeap(index);
	device->CreateDepthStencilView(resource, nullptr, cpuDescriptorHandle);

	return index;
}