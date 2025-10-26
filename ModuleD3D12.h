#pragma once
#include "Globals.h"
#include "Module.h"

#define FRAME_BUFFER_NUM 2
#define COLOR_CHANGE_RATE_CYCLE 120

class ModuleD3D12 : public Module {
public:
	ModuleD3D12(HWND _hWnd);
	bool init() override;
	void render() override;
	void preRender() override;
	void postRender() override;
	void WaitForFence(unsigned int fenceValue);
private:
	HWND hWnd;
	ComPtr<IDXGIAdapter4> adapter;
	ComPtr<ID3D12Device4> device;
	ComPtr<ID3D12CommandAllocator> commandAllocators[FRAME_BUFFER_NUM];
	ComPtr<ID3D12GraphicsCommandList> commandLists[FRAME_BUFFER_NUM];
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<IDXGISwapChain4> swapChain;
	ComPtr<ID3D12Resource2> buffers[FRAME_BUFFER_NUM];
	ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandles[FRAME_BUFFER_NUM];
	unsigned int currentBackBufferIndex = 0;
	ComPtr<ID3D12Fence1> fence;
	unsigned int fenceValue = 0;
	unsigned int fenceValues[FRAME_BUFFER_NUM];
	HANDLE fenceEvent;
	float red = 1.0f;
	float green = 0.0f;
	float blue = 0.0f;
};