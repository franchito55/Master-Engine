#include "Globals.h"
#include "ModuleBuffer.h"
#include "Application.h"
#include "ModuleD3D12.h"

extern Application* app;

ModuleBuffer::ModuleBuffer(HWND _hWnd) : hWnd(_hWnd) {}

bool ModuleBuffer::init() {
	return true;
}

void ModuleBuffer::createUploadBuffer(ComPtr<ID3D12Resource>& resourceHandle, const unsigned int bufferSize) {
	// Staging buffer -> heap type = UPLOAD (this buffer is used to upload data to the GPU)
	CD3DX12_HEAP_PROPERTIES sbHeapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC sbResDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
	HRESULT hr = app->getModuleD3D12()->getDevice()->CreateCommittedResource(&sbHeapProps, D3D12_HEAP_FLAG_NONE, &sbResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resourceHandle));
}

void ModuleBuffer::createDefaultBuffer(ComPtr<ID3D12Resource>& resourceHandle, const unsigned int bufferSize) {
	// Default buffer -> heap type = DEFAULT (this buffer will be read by the GPU)
	CD3DX12_HEAP_PROPERTIES vbHeapProps(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC vbResDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
	app->getModuleD3D12()->getDevice()->CreateCommittedResource(&vbHeapProps, D3D12_HEAP_FLAG_NONE, &vbResDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resourceHandle));
}

void ModuleBuffer::createDefaultBuffer(ComPtr<ID3D12Resource>& resourceHandle, D3D12_RESOURCE_DESC &bufferDesc) {
	// Default buffer -> heap type = DEFAULT (this buffer will be read by the GPU)
	CD3DX12_HEAP_PROPERTIES vbHeapProps(D3D12_HEAP_TYPE_DEFAULT);
	app->getModuleD3D12()->getDevice()->CreateCommittedResource(&vbHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resourceHandle));
}