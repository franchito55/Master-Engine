#include "Globals.h"
#include "Module.h"
#include "DirectXTex.h"

class ModuleResources : public Module {
public:
	ModuleResources(HWND _hWnd) : hWnd(_hWnd) {}
	~ModuleResources() {}

	bool init() override;

	void createUploadBuffer(ComPtr<ID3D12Resource>& resourceHandle, const unsigned int bufferSize); // Resource passed as reference
	void createUploadBufferWithData(ComPtr<ID3D12Resource>& resourceHandle, const void* data, const unsigned int bufferSize); // Resource passed as reference
	ComPtr<ID3D12Resource> createUploadBuffer(const unsigned int bufferSize); // Resource created in function and returned
	void createDefaultBuffer(ComPtr<ID3D12Resource>& resourceHandle, const unsigned int bufferSize); // Resource passed as reference
	void createDefaultBufferWithData(ComPtr<ID3D12Resource>& resourceHandle, const void* data, const unsigned int bufferSize); // Version with copying data in same function
	ComPtr<ID3D12Resource> createDefaultBuffer(const unsigned int bufferSize); // Resource created in function and returned
	void createDefaultBuffer(ComPtr<ID3D12Resource>& resourceHandle, D3D12_RESOURCE_DESC& bufferDesc);
	void copyDataToUploadBuffer(ComPtr<ID3D12Resource>& resourceHandle, const void* data, const size_t size);
	void copyDataFromUploadBufferToDefaultBuffer(ComPtr<ID3D12Resource>& defaultBufferHandle, ComPtr<ID3D12Resource>& uploadBufferHandle);
	ScratchImage createTextureFromFile(const std::string& path, ComPtr<ID3D12Resource>& defaultBufferHandle, ComPtr<ID3D12Resource>& uploadBufferHandle);

private:
	HWND hWnd;
	ComPtr<ID3D12Device2> device;
};