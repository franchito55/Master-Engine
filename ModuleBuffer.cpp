#include "Globals.h"
#include "ModuleBuffer.h"
#include "Application.h"
#include "ModuleD3D12.h"

extern Application* app;

ModuleBuffer::ModuleBuffer(HWND _hWnd) : hWnd(_hWnd) {}

bool ModuleBuffer::init() {
	// ============ Init staging buffer ============
	createUploadBuffer(stagingBuffer, BUFFER_SIZE);

	// ============ Init vertex buffer ============
	createDefaultBuffer(vertexBuffer.Get(), BUFFER_SIZE);

	return true;
}

void ModuleBuffer::preRender() {
	// Get a pointer to the resource in CPU (pData)
	BYTE* pData = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	stagingBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pData));

	// Copy the data from the CPU array to the Resource
	memcpy(pData, vertexArray, sizeof(vertexArray) * sizeof(float));

	// Invalidates the pointer -> probably marks it as "used" ???
	stagingBuffer->Unmap(0, nullptr);
}

void ModuleBuffer::render() {
	// Record the "move this data to GPU" command
	app->getModuleD3D12()->getCurrentBufferCommandList()->CopyResource(vertexBuffer.Get(), stagingBuffer.Get());
}

void ModuleBuffer::createUploadBuffer(ComPtr<ID3D12Resource> resourceHandle, const unsigned int bufferSize) {
	// Staging buffer -> heap type = UPLOAD (this buffer is used to upload data to the GPU)
	CD3DX12_HEAP_PROPERTIES sbHeapProps(D3D12_HEAP_TYPE_GPU_UPLOAD);
	CD3DX12_RESOURCE_DESC sbResDesc = CD3DX12_RESOURCE_DESC::Buffer(BUFFER_SIZE * sizeof(float));
	float defaultClearValue[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	CD3DX12_CLEAR_VALUE sbClearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, defaultClearValue);
	app->getModuleD3D12()->getDevice()->CreateCommittedResource(&sbHeapProps, D3D12_HEAP_FLAG_NONE, &sbResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, &sbClearValue, IID_PPV_ARGS(&resourceHandle));
}

void ModuleBuffer::createDefaultBuffer(ComPtr<ID3D12Resource> resourceHandle, const unsigned int bufferSize) {
	// Default buffer -> heap type = DEFAULT (this buffer will be read by the GPU)
	CD3DX12_HEAP_PROPERTIES vbHeapProps(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC vbResDesc = CD3DX12_RESOURCE_DESC::Buffer(BUFFER_SIZE * sizeof(float));
	float defaultClearValue[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	CD3DX12_CLEAR_VALUE sbClearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, defaultClearValue);
	app->getModuleD3D12()->getDevice()->CreateCommittedResource(&vbHeapProps, D3D12_HEAP_FLAG_NONE, &vbResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, &sbClearValue, IID_PPV_ARGS(&resourceHandle));
}