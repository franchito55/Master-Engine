#include "Globals.h"
#include "ModuleD3D12.h"
#include "ImGuiPass.h"

extern Application* app;

ModuleD3D12::ModuleD3D12(HWND _hWnd) : hWnd(_hWnd) {}

bool ModuleD3D12::init() {
	app->setModuleD3D12(this);

	// enable debug layer
#ifdef _DEBUG
	ComPtr<ID3D12Debug> iDebug;
	D3D12GetDebugInterface(IID_PPV_ARGS(&iDebug));
	iDebug->EnableDebugLayer();
#endif

	ComPtr<IDXGIFactory6> factory;
#if defined(_DEBUG)
	CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory));
#else
	CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
#endif

	// ============ Init device ============
	factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter));
	D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device));

	// ============ Init render command allocators ============ 
	for (int i = 0; i < FRAME_BUFFER_NUM; i++) {
		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&renderCommandAllocators[i]));
		renderCommandAllocators[i]->Reset();
	}

	// ============ Init copy command allocator ============ 
	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&copyCommandAllocator));

	// ============ Init render command lists ============ 
	for (int i = 0; i < FRAME_BUFFER_NUM; i++) {
		device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, renderCommandAllocators[i].Get(), nullptr, IID_PPV_ARGS(&renderCommandLists[i]));
		renderCommandLists[i]->Close();
	}

	// ============ Init copy command list ============
	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, copyCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&copyCommandList));

	// ============ Init render command queue ============ 
	D3D12_COMMAND_QUEUE_DESC renderQueueDesc = {};
	renderQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	renderQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	renderQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	renderQueueDesc.NodeMask = 0;
	device->CreateCommandQueue(&renderQueueDesc, IID_PPV_ARGS(&renderCommandQueue));

	// ============ Init copy command queue ============ 
	D3D12_COMMAND_QUEUE_DESC copyQueueDesc = {};
	copyQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	copyQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	copyQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	copyQueueDesc.NodeMask = 0;
	device->CreateCommandQueue(&copyQueueDesc, IID_PPV_ARGS(&copyCommandQueue));

	// ============ Init swap chain ============
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	RECT hWndRect;
	// Width and Height (buffer size)
	GetClientRect(hWnd, &hWndRect);
	if (&hWndRect) {
		swapChainDesc.Width = hWndRect.right - hWndRect.left;
		swapChainDesc.Height = hWndRect.bottom - hWndRect.top;
	}
	else {
		swapChainDesc.Width = 1920;
		swapChainDesc.Height = 1080;
	}
	// Color format (8b R, 8b G, 8b B, Normalized to 0.0-1.0)
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	// This is for VR
	swapChainDesc.Stereo = FALSE;
	// Multi-Sampling
	swapChainDesc.SampleDesc = { 1, 0 };
	// What the buffers will be used for
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	// How many buffers
	swapChainDesc.BufferCount = FRAME_BUFFER_NUM;
	// How scaling the window will behave
	swapChainDesc.Scaling = DXGI_SCALING_NONE;
	// Method of swapping front and back buffers
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	// How Alpha channel is treated
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	// Flags
	swapChainDesc.Flags = 0;
	factory->CreateSwapChainForHwnd(renderCommandQueue.Get(), hWnd, &swapChainDesc, nullptr, nullptr, (IDXGISwapChain1**)IID_PPV_ARGS_Helper(&swapChain));

	// ============ Init descriptor heap ============ 
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	// Type = Render Target View
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	// How many RTVs (2 -> 1 back buffer, 1 front buffer)
	descriptorHeapDesc.NumDescriptors = FRAME_BUFFER_NUM;
	// Shader-visible ?
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	// Flags for multi-adapter. Which adapter this descriptor is for
	descriptorHeapDesc.NodeMask = 0;
	device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));

	// ============ Init descriptors ============
	for (int i = 0; i < FRAME_BUFFER_NUM; i++) {
		swapChain->GetBuffer(i, IID_PPV_ARGS(&buffers[i]));
		D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuHandle;
		CD3DX12_CPU_DESCRIPTOR_HANDLE::InitOffsetted(rtvCpuHandle, descriptorHeap.Get()->GetCPUDescriptorHandleForHeapStart(), i, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
		device->CreateRenderTargetView(buffers[i].Get(), nullptr, rtvCpuHandle);
		rtvDescriptorHandles[i] = rtvCpuHandle;
	}

	// ============ Init fence ============
	device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	// ============ Init fence event (we'll use the same event for both fences) ============
	fenceEvent = CreateEventA(nullptr, FALSE, 0, nullptr);

	imGuiPass = new ImGuiPass(device.Get(), hWnd, {0}, {0});

	return true;
}

void ModuleD3D12::preRender() {
	// preRender de ModuleEditor
	imGuiPass->startFrame();

	// Get the current back buffer index
	currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();

	if (resizePending) {
		// When resizing, we need to wait for ALL buffers to be ready
		for (unsigned int i = 0; i < FRAME_BUFFER_NUM; i++) {
			WaitForFence(fenceValues[i]);
		}
		resizeBuffers();
		resizePending = false;
	}
	else {
		// Wait for the GPU to finish presenting the last frame
		WaitForFence(fenceValues[currentBackBufferIndex]);
	}

	//WaitForFence(fenceValues[currentBackBufferIndex]);

	// Reset allocator for currentBackBufferIndex
	renderCommandAllocators[currentBackBufferIndex]->Reset();

	// Reset the command list -> sets it to recording state
	renderCommandLists[currentBackBufferIndex]->Reset(renderCommandAllocators[currentBackBufferIndex].Get(), nullptr);

	// Set the usage state of the current buffer to RENDER_TARGET
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(buffers[currentBackBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE);
	renderCommandLists[currentBackBufferIndex]->ResourceBarrier(1, &barrier);

	renderCommandLists[currentBackBufferIndex]->ClearRenderTargetView(rtvDescriptorHandles[currentBackBufferIndex], color, 0, nullptr);
}

void ModuleD3D12::render() {
}

void ModuleD3D12::postRender() {
	// ImGui stuff in postRender() so it renders AFTER everything else
	ImGui::Begin("FPS info");
	// ImGui::LabelText(std::to_string(deltaTime.count()).c_str(), "deltaTime");
	unsigned int index = fpsCount % FPS_PLOTTING_MAX;
	frameTimes[index] = deltaTime.count() / 10000.0f;
	fps[index] = 1000.0f / frameTimes[index];

	if (index == 0) {
		minFps = 99999;
		maxFps = 0;
	}
	if (fps[index] < minFps)
		minFps = fps[index];
	if (fps[index] > maxFps)
		maxFps = fps[index];
	unsigned int averageFps = 0;
	unsigned int numFps = 0;
	for (unsigned int i = 0; i < FPS_PLOTTING_MAX; i++) {
		if (fps[i] != 0 && fps[i] != INFINITE) {
			numFps++;
			averageFps += fps[i];
		}
	}
	averageFps /= numFps;

	char overlay[32];
	snprintf(overlay, 32, "avg: %d min: %d max: %d", averageFps, minFps, maxFps);
	ImGui::PlotLines("Frame times", frameTimes, IM_ARRAYSIZE(frameTimes), 0, (std::to_string((int)frameTimes[index]) + " ms").c_str(), 0.0f, 32.0f, ImVec2(0, 80.0f));
	ImGui::PlotLines("FPS", fps, IM_ARRAYSIZE(fps), 0, overlay, 0.0f, 360.0f, ImVec2(0, 80.0f));
	ImGui::End();

	// This HAS to go last so that the UI gets rendered on top
	imGuiPass->record(renderCommandLists[currentBackBufferIndex].Get(), rtvDescriptorHandles[currentBackBufferIndex]);

	// Transition back to PRESENT
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(buffers[currentBackBufferIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE);
	renderCommandLists[currentBackBufferIndex]->ResourceBarrier(1, &barrier);

	// Close list and execute command list
	renderCommandLists[currentBackBufferIndex]->Close();
	ID3D12CommandList* lists[] = { renderCommandLists[currentBackBufferIndex].Get() };
	renderCommandQueue->ExecuteCommandLists(1, lists);

	// Present
	swapChain->Present(1, 0);

	// This tells the GPU to set the fence's value to what you pass
	fpsCount++;
	fenceValues[currentBackBufferIndex] = fpsCount;
	renderCommandQueue->Signal(fence.Get(), fenceValues[currentBackBufferIndex]);
}

void ModuleD3D12::WaitForFence(const unsigned int _fenceValue) {
	// Set fenceEvent as "completed" when the fence's value == frameCounter
	fence->SetEventOnCompletion(_fenceValue, fenceEvent);
	// Wait for the event to be "completed" (it means the GPU has increased the fence value by 1 and is done processing the frame)
	WaitForSingleObject(fenceEvent, INFINITE);
	deltaTime = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()) - lastFrameTime;
	lastFrameTime = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
}

void ModuleD3D12::setResizePending(const RECT &_resizedRect) {
	resizePending = true;
	resizedRect = _resizedRect;
}

void ModuleD3D12::resizeBuffers() {

	// Release swap chain buffers
	for (unsigned int i = 0; i < FRAME_BUFFER_NUM; i++) {
		buffers[i].Reset();
	}

	swapChain->ResizeBuffers(FRAME_BUFFER_NUM, resizedRect.right - resizedRect.left, resizedRect.bottom - resizedRect.top, DXGI_FORMAT_UNKNOWN, 0);
	for (unsigned int i = 0; i < FRAME_BUFFER_NUM; i++) {
		swapChain->GetBuffer(i, IID_PPV_ARGS(&buffers[i]));
		D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuHandle;
		CD3DX12_CPU_DESCRIPTOR_HANDLE::InitOffsetted(rtvCpuHandle, descriptorHeap.Get()->GetCPUDescriptorHandleForHeapStart(), i, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
		device->CreateRenderTargetView(buffers[i].Get(), nullptr, rtvCpuHandle);
		rtvDescriptorHandles[i] = rtvCpuHandle;
	}
	
	// swapChain->ResizeBuffers() RESETS the swap chain's current back buffer index 
	// back to 0, so we need to update it here
	currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();
	for (unsigned int i = 0; i < FRAME_BUFFER_NUM; i++) {
		fenceValues[i] = 0;
	}
}