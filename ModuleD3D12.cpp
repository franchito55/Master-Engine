#include "Globals.h"
#include "ModuleD3D12.h"
#include "ModuleBuffer.h"

extern Application* app;

ModuleD3D12::ModuleD3D12(HWND _hWnd) : hWnd(_hWnd) {}

bool ModuleD3D12::init() {

	app->setModuleD3D12(this);

	enableDebugLayer();

	const ComPtr<IDXGIFactory6> factory = initDevice();

	initCommandAllocators();

	initCommandLists();

	initCommandQueues();

	initSwapChain(factory);

	initDescriptorHeaps();

	initDescriptors();
	
	// ============ Init fence ============
	device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	// ============ Init fence event (we'll use the same event for both fences) ============
	fenceEvent = CreateEventA(nullptr, FALSE, 0, nullptr);

	initDSB();

	return true;
}

void ModuleD3D12::preRender() {

	// Get the current back buffer index
	currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();

	if (resizePending) {
		// When resizing, we need to wait for ALL buffers to be ready
		WaitForAllFences();
		resizeBuffers();
		resizePending = false;
	}
	else {
		frameIndex = (frameIndex + 1) % FRAME_BUFFER_NUM;
		WaitForFence(fenceValues[frameIndex]);
	}

	//WaitForFence(fenceValues[currentBackBufferIndex]);

	// Reset allocator for currentBackBufferIndex
	renderCommandAllocators[currentBackBufferIndex]->Reset();

	// Reset the command list -> sets it to recording state
	renderCommandLists[currentBackBufferIndex]->Reset(renderCommandAllocators[currentBackBufferIndex].Get(), nullptr);

	// Set the usage state of the current buffer to RENDER_TARGET
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(buffers[currentBackBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE);
	renderCommandLists[currentBackBufferIndex]->ResourceBarrier(1, &barrier);

	// Set depth buffer view's state back to WRITE
	depthBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(depthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	//renderCommandLists[currentBackBufferIndex]->ResourceBarrier(1, &depthBufferBarrier);
	renderCommandLists[currentBackBufferIndex]->ClearDepthStencilView(dsvDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0.0f, 0, nullptr);

	renderCommandLists[currentBackBufferIndex]->ClearRenderTargetView(rtvDescriptorHandles[currentBackBufferIndex], color, 0, nullptr);
}

void ModuleD3D12::render() {
}

void ModuleD3D12::postRender() {
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
	fenceValues[currentBackBufferIndex] = frameCount++;
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
	// TODO: resize depth/stencil buffer
	
	// Release swap chain buffers
	for (unsigned int i = 0; i < FRAME_BUFFER_NUM; i++) {
		buffers[i].Reset();
	}
	// Release DSV
	depthStencilBuffer.Reset();

	swapChain->ResizeBuffers(FRAME_BUFFER_NUM, resizedRect.right - resizedRect.left, resizedRect.bottom - resizedRect.top, DXGI_FORMAT_UNKNOWN, 0);
	for (unsigned int i = 0; i < FRAME_BUFFER_NUM; i++) {
		swapChain->GetBuffer(i, IID_PPV_ARGS(&buffers[i]));
		CD3DX12_CPU_DESCRIPTOR_HANDLE::InitOffsetted(rtvDescriptorHandles[i], descriptorHeap.Get()->GetCPUDescriptorHandleForHeapStart(), i, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
		device->CreateRenderTargetView(buffers[i].Get(), nullptr, rtvDescriptorHandles[i]);
	}

	// Re-create DSV
	CD3DX12_HEAP_PROPERTIES dsbHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC dsbHeapDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, app->getWindowWidth(), app->getWindowHeight(), 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;
	device->CreateCommittedResource(&dsbHeapProps, D3D12_HEAP_FLAG_NONE, &dsbHeapDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&depthStencilBuffer));
	CD3DX12_CPU_DESCRIPTOR_HANDLE::InitOffsetted(dsvDescriptorHandle, depthBufferDescriptorHeap.Get()->GetCPUDescriptorHandleForHeapStart(), 0, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV));
	device->CreateDepthStencilView(depthStencilBuffer.Get(), nullptr, dsvDescriptorHandle);
	
	// swapChain->ResizeBuffers() RESETS the swap chain's current back buffer index 
	// back to 0, so we need to update it here
	currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();
	for (unsigned int i = 0; i < FRAME_BUFFER_NUM; i++) {
		fenceValues[i] = 0;
	}
}

ComPtr<IDXGIFactory6> ModuleD3D12::initDevice() {
	ComPtr<IDXGIFactory6> factory;
#if defined(_DEBUG)
	CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory));
#else
	CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
#endif

	// ============ Init device ============
	factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter));
	D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device));
	
	return factory;
}

void ModuleD3D12::initCommandAllocators() {
	// ============ Init render command allocators ============ 
	for (int i = 0; i < FRAME_BUFFER_NUM; i++) {
		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&renderCommandAllocators[i]));
		renderCommandAllocators[i]->Reset();
	}

	// ============ Init copy command allocator ============ 
	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&copyCommandAllocator));
}

void ModuleD3D12::initCommandLists() {
	// ============ Init render command lists ============ 
	for (int i = 0; i < FRAME_BUFFER_NUM; i++) {
		device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, renderCommandAllocators[i].Get(), nullptr, IID_PPV_ARGS(&renderCommandLists[i]));
		// need the first one open in order to init the texture buffer
		if (i > 0)
			renderCommandLists[i]->Close();
	}

	// ============ Init copy command list ============
	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, copyCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&copyCommandList));
}

void ModuleD3D12::initCommandQueues() {
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
}

void ModuleD3D12::initSwapChain(ComPtr<IDXGIFactory6> factory) {
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
}

void ModuleD3D12::initDescriptorHeaps() {
	// ============ Init descriptor heap (non-shader visible) ============ 
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

	// ============ Init descriptor heap for shader visible (CBV, SRV, UAV) ============
	D3D12_DESCRIPTOR_HEAP_DESC shaderVisibleDescriptorHeapDesc = {};
	// Type = Render Target View
	shaderVisibleDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	// How many RTVs (2 -> 1 back buffer, 1 front buffer)
	shaderVisibleDescriptorHeapDesc.NumDescriptors = SHADER_VISIBLE_DESCRIPTOR_NUMBER;
	// Shader-visible ?
	shaderVisibleDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	// Flags for multi-adapter. Which adapter this descriptor is for
	shaderVisibleDescriptorHeapDesc.NodeMask = 0;
	device->CreateDescriptorHeap(&shaderVisibleDescriptorHeapDesc, IID_PPV_ARGS(&shaderVisibleDescriptorHeap));

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
	device->CreateDescriptorHeap(&dsbDescriptorHeap, IID_PPV_ARGS(&depthBufferDescriptorHeap));
}

void ModuleD3D12::initDescriptors() {
	// ============ Init descriptors ============
	for (int i = 0; i < FRAME_BUFFER_NUM; i++) {
		swapChain->GetBuffer(i, IID_PPV_ARGS(&buffers[i]));
		CD3DX12_CPU_DESCRIPTOR_HANDLE::InitOffsetted(rtvDescriptorHandles[i], descriptorHeap.Get()->GetCPUDescriptorHandleForHeapStart(), i, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
		device->CreateRenderTargetView(buffers[i].Get(), nullptr, rtvDescriptorHandles[i]);
	}
}

void ModuleD3D12::initDSB() {
	// ============ Init depth & stencil buffers ============
	CD3DX12_HEAP_PROPERTIES dsbHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC dsbHeapDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, app->getWindowWidth(), app->getWindowHeight(), 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;
	device->CreateCommittedResource(&dsbHeapProps, D3D12_HEAP_FLAG_NONE, &dsbHeapDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&depthStencilBuffer));
}

void ModuleD3D12::initDSV() {
	// ============ Init DSV (depth/stencil view) ============
	CD3DX12_CPU_DESCRIPTOR_HANDLE::InitOffsetted(dsvDescriptorHandle, depthBufferDescriptorHeap.Get()->GetCPUDescriptorHandleForHeapStart(), 0, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV));
	device->CreateDepthStencilView(depthStencilBuffer.Get(), nullptr, dsvDescriptorHandle);
}

void ModuleD3D12::enableDebugLayer() {
	// enable debug layer
#ifdef _DEBUG
	ComPtr<ID3D12Debug> iDebug;
	D3D12GetDebugInterface(IID_PPV_ARGS(&iDebug));
	iDebug->EnableDebugLayer();
#endif
}

void ModuleD3D12::WaitForAllFences() {
	for (unsigned int i = 0; i < FRAME_BUFFER_NUM; i++) {
		WaitForFence(fenceValues[i]);
	}
}