#pragma once
#include "Globals.h"
#include "Module.h"

class ModuleShaderDescriptors : public Module {
public:
	ModuleShaderDescriptors(HWND _hWnd) : hWnd(_hWnd) {}
	~ModuleShaderDescriptors() {}

	bool init() override;

	ComPtr<ID3D12DescriptorHeap> getDescriptorHeap() const { return genericSrvHeap; }

	unsigned int createGenericSRV(ID3D12Resource* resource, DXGI_FORMAT format, unsigned int mipLevels);
	unsigned int createSampler(D3D12_SAMPLER_DESC* samplerDesc);
	unsigned int allocateDescriptor();
	D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandleFromGenericHeap(const unsigned int index) const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(genericSrvHeap->GetCPUDescriptorHandleForHeapStart(), index, genericDescriptorSize); }
	D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandleFromGenericHeap(const unsigned int index) const { return CD3DX12_GPU_DESCRIPTOR_HANDLE(genericSrvHeap->GetGPUDescriptorHandleForHeapStart(), index, genericDescriptorSize); }
	D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandleFromSamplerHeap(const unsigned int index) const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(samplerHeap->GetCPUDescriptorHandleForHeapStart(), index, samplerDescriptorSize); }
	D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandleFromSamplerHeap(const unsigned int index) const { return CD3DX12_GPU_DESCRIPTOR_HANDLE(samplerHeap->GetGPUDescriptorHandleForHeapStart(), index, samplerDescriptorSize); }
	void reset() { 
		nextFreeGenericIndex = 0; 
		nextFreeSamplerIndex = 0; 
	} // Not sure this is correct, shouldn't we verify that it is indeed free? -> delegate responsibility to whoever calls it?

private:
	HWND hWnd;
	ComPtr<ID3D12DescriptorHeap> genericSrvHeap;
	ComPtr<ID3D12DescriptorHeap> samplerHeap;
	ComPtr<ID3D12Device2> device;
	unsigned int nextFreeGenericIndex = 0;
	unsigned int nextFreeSamplerIndex = 0;
	unsigned int genericDescriptorSize = 0;
	unsigned int samplerDescriptorSize = 0;

	void createDefaultSamplers();
};