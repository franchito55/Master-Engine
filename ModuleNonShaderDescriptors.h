#include "Globals.h"
#include "Module.h"

class ModuleNonShaderDescriptors : public Module {
public:
	ModuleNonShaderDescriptors(HWND _hWnd) : hWnd(_hWnd) {}
	~ModuleNonShaderDescriptors() {}

	bool init() override;

	ID3D12DescriptorHeap* getRTVHeap() { return rtvHeap.Get(); }
	ID3D12DescriptorHeap* getDSVHeap() { return dsvHeap.Get(); }

	unsigned int createRTV(ID3D12Resource* resource);
	unsigned int createRTV(ID3D12Resource* resource, D3D12_RENDER_TARGET_VIEW_DESC& desc);
	unsigned int createDSV(ID3D12Resource* resource);
	unsigned int allocateDescriptor();
	D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandleFromRTVHeap(const unsigned int index) const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUDescriptorHandleForHeapStart(), index, rtvDescriptorSize); }
	D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandleFromDSVHeap(const unsigned int index) const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvHeap->GetCPUDescriptorHandleForHeapStart(), index, dsvDescriptorSize); }
	void reset() { 
		rtvHeapNextFreeIndex = 0;
		dsvHeapNextFreeIndex = 0;
	} // Not sure this is correct, shouldn't we verify that it is indeed free? -> delegate responsibility to whoever calls it?

private:
	HWND hWnd;
	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	ComPtr<ID3D12DescriptorHeap> dsvHeap;
	ComPtr<ID3D12Device2> device;
	unsigned int rtvHeapNextFreeIndex = 0;
	unsigned int dsvHeapNextFreeIndex = 0;
	unsigned int rtvDescriptorSize = 0;
	unsigned int dsvDescriptorSize = 0;
};