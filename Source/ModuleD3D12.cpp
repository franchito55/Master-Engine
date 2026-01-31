#include "Globals.h"
#include "ModuleD3D12.h"
#include "ModuleNonShaderDescriptors.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleImGui.h"

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
	
	// ============ Init fence ============
	device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	// ============ Init fence event (we'll use the same event for both fences) ============
	fenceEvent = CreateEventA(nullptr, FALSE, 0, nullptr);

	return true;
}

// We need ModuleNonShaderDescriptors to have run its init, but ModuleNonShaderDescriptors needs ModuleD3D12 to have run its init...
bool ModuleD3D12::postInit() {
	recreateRTVs();
	dsvIndex = app->getModuleNonShaderDescriptors()->createDSV(depthStencilBuffer.Get());

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

	// Reset allocator for currentBackBufferIndex
	renderCommandAllocators[currentBackBufferIndex]->Reset();

	// Reset the command list -> sets it to recording state
	renderCommandLists[currentBackBufferIndex]->Reset(renderCommandAllocators[currentBackBufferIndex].Get(), nullptr);

	// Set the usage state of the current buffer to RENDER_TARGET
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffers[currentBackBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE);
	renderCommandLists[currentBackBufferIndex]->ResourceBarrier(1, &barrier);

	// Set depth buffer view's state back to WRITE
	depthBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(depthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	//renderCommandLists[currentBackBufferIndex]->ResourceBarrier(1, &depthBufferBarrier);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle = app->getModuleNonShaderDescriptors()->getCPUHandleFromDSVHeap(dsvIndex);
	renderCommandLists[currentBackBufferIndex]->ClearDepthStencilView(dsvCpuHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0.0f, 0, nullptr);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuHandle = app->getModuleNonShaderDescriptors()->getCPUHandleFromRTVHeap(rtvIndices[currentBackBufferIndex]);
	renderCommandLists[currentBackBufferIndex]->ClearRenderTargetView(rtvCpuHandle, color, 0, nullptr);
}

void ModuleD3D12::render() {
}

void ModuleD3D12::postRender() {
	// Transition back to PRESENT
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffers[currentBackBufferIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE);
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

void ModuleD3D12::flush() {
	fenceValues[currentBackBufferIndex] += FRAME_BUFFER_NUM;
	renderCommandQueue->Signal(fence.Get(), fenceValues[currentBackBufferIndex]);
	// Set fenceEvent as "completed" when the fence's value == frameCounter
	fence->SetEventOnCompletion(fenceValues[currentBackBufferIndex], fenceEvent);
	// Wait for the event to be "completed" (it means the GPU has increased the fence value by 1 and is done processing the frame)
	WaitForSingleObject(fenceEvent, INFINITE);
	deltaTime = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()) - lastFrameTime;
	lastFrameTime = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
}

// Flushes a specific command queue (for example the copy one)
void ModuleD3D12::flush(ID3D12CommandQueue* commandQueue) {
	fenceValues[currentBackBufferIndex] += FRAME_BUFFER_NUM;
	commandQueue->Signal(fence.Get(), fenceValues[currentBackBufferIndex]);
	// Set fenceEvent as "completed" when the fence's value == frameCounter
	fence->SetEventOnCompletion(fenceValues[currentBackBufferIndex], fenceEvent);
	// Wait for the event to be "completed" (it means the GPU has increased the fence value by 1 and is done processing the frame)
	WaitForSingleObject(fenceEvent, INFINITE);
}

void ModuleD3D12::WaitForFence(const unsigned int _fenceValue) {
	// Set fenceEvent as "completed" when the fence's value == frameCounter
	fence->SetEventOnCompletion(_fenceValue, fenceEvent);
	// Wait for the event to be "completed" (it means the GPU has increased the fence value by 1 and is done processing the frame)
	WaitForSingleObject(fenceEvent, INFINITE);
	deltaTime = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()) - lastFrameTime;
	lastFrameTime = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
}

void ModuleD3D12::setWindowResizePending(const RECT &_resizedRect) {
	resizePending = true;
	windowResizedRect = _resizedRect;
}

void ModuleD3D12::setSceneResizePending(const RECT &_resizedRect) {
	resizePending = true;
	sceneResizedRect = _resizedRect;
}

void ModuleD3D12::resizeBuffers() {
	
	// Release RTVs
	for (unsigned int i = 0; i < FRAME_BUFFER_NUM; i++) {
		backBuffers[i].Reset();
	}
	// Release DSV
	depthStencilBuffer.Reset();

	app->getModuleNonShaderDescriptors()->reset();

	swapChain->ResizeBuffers(FRAME_BUFFER_NUM, windowResizedRect.right - windowResizedRect.left, windowResizedRect.bottom - windowResizedRect.top, DXGI_FORMAT_UNKNOWN, 0);
	recreateRTVs();

	// Re-create DSV
	app->getModuleNonShaderDescriptors()->createDSV(depthStencilBuffer.Get());
	
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

void ModuleD3D12::recreateRTVs() {
	for (UINT i = 0; i < FRAME_BUFFER_NUM; ++i)
	{
		swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i]));
		rtvIndices[i] = app->getModuleNonShaderDescriptors()->createRTV(backBuffers[i].Get());
	}
	float sceneRTVTextureWidth = sceneResizedRect.right - sceneResizedRect.left;
	float sceneRTVTextureHeight = sceneResizedRect.bottom - sceneResizedRect.top;
	if (sceneRTVTextureWidth == 0 || sceneRTVTextureHeight == 0) { // Fallback
		sceneRTVTextureWidth = 400;
		sceneRTVTextureHeight = 400;
	}
	D3D12_RESOURCE_DESC sceneRTVTextureDesc = {};
	sceneRTVTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	sceneRTVTextureDesc.Alignment = 0;
	sceneRTVTextureDesc.Width = sceneRTVTextureWidth;
	sceneRTVTextureDesc.Height = sceneRTVTextureHeight;
	sceneRTVTextureDesc.DepthOrArraySize = 1;
	sceneRTVTextureDesc.MipLevels = 1;
	sceneRTVTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sceneRTVTextureDesc.SampleDesc.Count = 1;
	sceneRTVTextureDesc.SampleDesc.Quality = 0;
	sceneRTVTextureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	sceneRTVTextureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	clearValue.Color[0] = 0.2f;
	clearValue.Color[1] = 0.2f;
	clearValue.Color[2] = 0.2f;
	clearValue.Color[3] = 1.0f;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	app->getModuleD3D12()->getDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &sceneRTVTextureDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue, IID_PPV_ARGS(&sceneRenderTexture));
	sceneSRVIndexInHeap = app->getModuleShaderDescriptors()->createGenericSRV(sceneRenderTexture.Get(), DXGI_FORMAT_R8G8B8A8_UNORM, 1);
	D3D12_RENDER_TARGET_VIEW_DESC sceneRTVDesc = {};
	sceneRTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sceneRTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	sceneRTVDesc.Texture2D.MipSlice = 0;
	sceneRTVIndexInHeap = app->getModuleNonShaderDescriptors()->createRTV(sceneRenderTexture.Get(), sceneRTVDesc);
}