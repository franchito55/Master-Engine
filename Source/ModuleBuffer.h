#pragma once
#include "Globals.h"
#include "Module.h"

#define BUFFER_SIZE 3

class ModuleBuffer : public Module {
public:
	ModuleBuffer(HWND _hWnd);
	bool init() override;
	void preRender() override;
	void render() override;
	static void createUploadBuffer(ComPtr<ID3D12Resource>& resourceHandle, const unsigned int bufferSize);
	static void createDefaultBuffer(ComPtr<ID3D12Resource>& resourceHandle, const unsigned int bufferSize);
private:
	HWND hWnd;
	ComPtr<ID3D12Device4> device;
	ComPtr<ID3D12Resource> stagingBuffer = nullptr;
	ComPtr<ID3D12Resource> vertexBuffer = nullptr;

	float vertexArray[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
};