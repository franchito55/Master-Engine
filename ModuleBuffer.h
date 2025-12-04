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
	void createDefaultBuffer(ComPtr<ID3D12Resource>& resourceHandle, D3D12_RESOURCE_DESC& bufferDesc);
private:
	HWND hWnd;
};