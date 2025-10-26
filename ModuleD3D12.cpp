#include "Globals.h"
#include "ModuleD3D12.h"

ModuleD3D12::ModuleD3D12(HWND _hWnd) : hWnd(_hWnd) {}

bool ModuleD3D12::init() {
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

	// ============ Init command allocators ============ 
	for (int i = 0; i < FRAME_BUFFER_NUM; i++) {
		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[i]));
	}

	// ============ Init command lists ============ 
	for (int i = 0; i < FRAME_BUFFER_NUM; i++) {
		device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&commandLists[i]));
	}

	// ============ Init command queue ============ 
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.NodeMask = 0;
	device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));

	// ============ Init swap chain ============
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	RECT hWndRect;
	// Width and Height (buffer size)
	GetWindowRect(hWnd, &hWndRect);
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
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	// Method of swapping front and back buffers
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	// How Alpha channel is treated
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	// Flags
	swapChainDesc.Flags = 0;
	factory->CreateSwapChainForHwnd(commandQueue.Get(), hWnd, &swapChainDesc, nullptr, nullptr, (IDXGISwapChain1**)IID_PPV_ARGS_Helper(&swapChain));

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

	// ============ Init fence values ============
	for (int i = 0; i < FRAME_BUFFER_NUM; i++) {
		fenceValues[i] = 0;
	}

	// ============ Init fence event (we'll use the same event for both fences) ============
	fenceEvent = CreateEventA(nullptr, FALSE, 0, nullptr);

	return true;
}

void ModuleD3D12::preRender() {
	// Get the current back buffer index
	currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();

	// Wait for the GPU to finish
	WaitForFence(fenceValues[currentBackBufferIndex]);

	// Reset allocator for currentBackBufferIndex
	commandAllocators[currentBackBufferIndex]->Reset();

	// Reset the command list -> sets it to recording state
	commandLists[currentBackBufferIndex]->Reset(commandAllocators[currentBackBufferIndex].Get(), nullptr);
}

void ModuleD3D12::render() {
	// Record commands. This involves:
	// 1.Set the usage state of the current buffer to RENDER_TARGET
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(buffers[currentBackBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE);
	commandLists[currentBackBufferIndex]->ResourceBarrier(1, &barrier);

	// 2. Record the actual commands (Reset, Draw, etc.)
	float fV = fenceValue % 720;
	if (fV > 120 && fV < 240) {
		red -= 1.0f / 120;
	}
	else if (fV > 480 && fV < 600) {
		red += 1.0f / 120;
	}

	if (fV > 0 && fV < 120) {
		green += 1.0f / 120;
	}
	else if (fV > 360 && fV < 480) {
		green -= 1.0f / 120;
	}

	if (fV > 240 && fV < 360) {
		blue += 1.0f / 120;
	}
	else if (fV > 600 && fV < 720) {
		blue -= 1.0f / 120;
	}

	float color[4] = { red, green, blue, 1.0f };
	commandLists[currentBackBufferIndex]->ClearRenderTargetView(rtvDescriptorHandles[currentBackBufferIndex], color, 0, nullptr);

	// 3. Transition back to PRESENT
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(buffers[currentBackBufferIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE);
	commandLists[currentBackBufferIndex]->ResourceBarrier(1, &barrier);

	// Close list and execute command list
	commandLists[currentBackBufferIndex]->Close();
	ID3D12CommandList* lists[] = { commandLists[currentBackBufferIndex].Get()};
	commandQueue->ExecuteCommandLists(1, lists);

	// Present
	swapChain->Present(1, 0);
}

void ModuleD3D12::postRender() {
	// This tells the GPU to set the fence's value to what you pass
	commandQueue->Signal(fence.Get(), ++fenceValue);
	currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();
	fenceValues[currentBackBufferIndex] = fenceValue; // Store the fence for this buffer
	
}

void ModuleD3D12::WaitForFence(unsigned int fenceValue) {
	// Set fenceEvent as "completed" when the fence's value == frameCounter
	fence->SetEventOnCompletion(fenceValue, fenceEvent);
	// Wait for the event to be "completed" (it means the GPU has increased the fence value by 1 and is done processing the frame)
	WaitForSingleObject(fenceEvent, INFINITE);
}