#pragma once
#include "Globals.h"
#include "Module.h"
#include <chrono>
#include "Application.h"

#define FRAME_BUFFER_NUM 3

class ModuleD3D12 : public Module {
	typedef struct ResizeStruct {
		unsigned int width = 0;
		unsigned int height = 0;
	};

public:
	ModuleD3D12(HWND _hWnd);
	bool init() override;
	bool postInit() override;
	void render() override;
	void preRender() override;
	void postRender() override;
	void WaitForFence(const unsigned int fenceValue);
	ComPtr<ID3D12Device2> getDevice() const { return device; }
	ComPtr<ID3D12GraphicsCommandList> getCurrentBufferCommandList() const { return renderCommandLists[currentBackBufferIndex]; }
	ComPtr<ID3D12CommandQueue> getRenderCommandQueue() const { return renderCommandQueue; }
	ComPtr<ID3D12CommandQueue> getCopyCommandQueue() const { return copyCommandQueue; }
	ComPtr<ID3D12GraphicsCommandList> getCopyCommandList() const { return copyCommandList; }
	ComPtr<ID3D12CommandAllocator> getCopyCommandAllocator() const { return copyCommandAllocator; }
	ComPtr<IDXGISwapChain4> getSwapChain() const { return swapChain; }
	ComPtr<ID3D12Resource2> getBuffer(unsigned int index) const { return backBuffers[index]; }
	void setWindowResizePending(const RECT &windowResizedRect);
	void setSceneResizePending(const RECT &sceneResizedRect);
	void resizeBuffers();
	void flush();
	void flush(ID3D12CommandQueue* commandQueue);
	unsigned int getCurrentRTVIndexInRTVHeap() const { return rtvIndices[currentBackBufferIndex]; }
	void setRTVIndex(const unsigned int index, const unsigned int rtvIndex) { rtvIndices[index] = rtvIndex; }
	ComPtr<ID3D12Fence> getFence() const { return fence; }
	ComPtr<ID3D12Resource2> getDepthStencilBuffer() const { return depthStencilBuffer; }
	unsigned int getDSVIndexInDSVHeap() const { return dsvIndex; }
	void WaitForAllFences();
	long long getDeltaTime() { return deltaTime.count(); }

	unsigned int getSceneRTVIndexInHeap() { return sceneRTVIndexInHeap; }
	unsigned int getSceneSRVIndexInHeap() { return sceneSRVIndexInHeap; }
	ID3D12Resource* getSceneRenderTexture() { return sceneRenderTexture.Get(); }

private:
	HWND hWnd;
	ComPtr<IDXGIAdapter4> adapter;
	ComPtr<ID3D12Device4> device;
	ComPtr<ID3D12CommandAllocator> renderCommandAllocators[FRAME_BUFFER_NUM];
	ComPtr<ID3D12CommandAllocator> copyCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> renderCommandLists[FRAME_BUFFER_NUM];
	ComPtr<ID3D12GraphicsCommandList> copyCommandList;
	ComPtr<ID3D12CommandQueue> renderCommandQueue;
	ComPtr<ID3D12CommandQueue> copyCommandQueue;
	ComPtr<IDXGISwapChain4> swapChain;
	ComPtr<ID3D12Resource2> backBuffers[FRAME_BUFFER_NUM];
	unsigned int rtvIndices[FRAME_BUFFER_NUM]; // Indices in the ModuleNonShaderDescriptor's heap
	unsigned int dsvIndex; // Index in the ModuleNonShaderDescriptor's heap
	ComPtr<ID3D12Resource2> depthStencilBuffer;
	unsigned int currentBackBufferIndex = 0;
	ComPtr<ID3D12Fence1> fence;
	unsigned int fenceValues[FRAME_BUFFER_NUM] = { 0, 0, 0 };
	HANDLE fenceEvent;
	float red = 1.0f;
	float green = 0.0f;
	float blue = 0.0f;

	CD3DX12_RESOURCE_BARRIER barrier = {};
	CD3DX12_RESOURCE_BARRIER depthBufferBarrier = {};

	// Rendering to texture
	ComPtr<ID3D12Resource> sceneRenderTexture;
	unsigned int sceneSRVIndexInHeap = 0;
	unsigned int sceneRTVIndexInHeap = 0;

	bool resizePending = false;
	RECT windowResizedRect = {};
	RECT sceneResizedRect = {};

	float color[3] = { 0.2f, 0.2f, 0.2f };

	unsigned int frameIndex = 0;

	unsigned int frameCount = 0;
	std::chrono::system_clock::duration deltaTime;
	std::chrono::system_clock::time_point lastFrameTime = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());

	void enableDebugLayer();
	ComPtr<IDXGIFactory6> initDevice();
	void initCommandAllocators();
	void initCommandLists();
	void initCommandQueues();
	void initSwapChain(const ComPtr<IDXGIFactory6> factory);
	void recreateRTVs();
};