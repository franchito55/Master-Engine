#pragma once
#include "Globals.h"
#include "Module.h"
#include <chrono>

class ImGuiPass;

#define FRAME_BUFFER_NUM 2
#define COLOR_CHANGE_RATE_CYCLE 120
#define FPS_PLOTTING_MAX 500

class ModuleD3D12 : public Module {
	typedef struct ResizeStruct {
		unsigned int width = 0;
		unsigned int height = 0;
	};

public:
	ModuleD3D12(HWND _hWnd);
	bool init() override;
	void render() override;
	void preRender() override;
	void postRender() override;
	void WaitForFence(unsigned int fenceValue);
	ComPtr<ID3D12Device2> getDevice() const { return device; }
	ComPtr<ID3D12GraphicsCommandList> getCurrentBufferCommandList() const { return commandLists[currentBackBufferIndex]; }
	void setResizePending(RECT &resizedRect);
	void resizeBuffers();
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
	HANDLE fenceEvent;
	float red = 1.0f;
	float green = 0.0f;
	float blue = 0.0f;

	CD3DX12_RESOURCE_BARRIER barrier = {};

	bool resizePending = false;
	RECT resizedRect = {};

	ImGuiPass* imGuiPass;

	float color[3] = { 0.2f, 0.2f, 0.2f };

	std::chrono::system_clock::duration deltaTime;
	std::chrono::system_clock::time_point lastFrameTime = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
	float frameTimes[FPS_PLOTTING_MAX] = {};
	float fps[FPS_PLOTTING_MAX] = {};

	unsigned int minFps = 99999;
	unsigned int maxFps = 0;
};