#include "Globals.h"
#include "ModuleResources.h"
#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleShaderDescriptors.h"

extern Application* app;

bool ModuleResources::init() {
	device = app->getModuleD3D12().getDevice();

	return true;
}

void ModuleResources::createDefaultBuffer(ComPtr<ID3D12Resource>& resourceHandle, const unsigned int bufferSize) {
	// Default buffer -> heap type = DEFAULT (this buffer will be read by the GPU)
	CD3DX12_HEAP_PROPERTIES vbHeapProps(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC vbResDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
	device->CreateCommittedResource(&vbHeapProps, D3D12_HEAP_FLAG_NONE, &vbResDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resourceHandle));
	app->getModuleD3D12().flush();
}

ComPtr<ID3D12Resource> ModuleResources::createDefaultBuffer(const unsigned int bufferSize) {
	ComPtr<ID3D12Resource> resourceHandle;
	// Default buffer -> heap type = DEFAULT (this buffer will be read by the GPU)
	CD3DX12_HEAP_PROPERTIES vbHeapProps(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC vbResDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
	device->CreateCommittedResource(&vbHeapProps, D3D12_HEAP_FLAG_NONE, &vbResDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resourceHandle));
	app->getModuleD3D12().flush();
	return resourceHandle;
}

void ModuleResources::createDefaultBufferWithData(ComPtr<ID3D12Resource>& resourceHandle, const void* data, const unsigned int bufferSize) {
	ComPtr<ID3D12Resource> tempUpload;
	createUploadBufferWithData(tempUpload, data, bufferSize);
	createDefaultBuffer(resourceHandle, bufferSize);
	copyDataFromUploadBufferToDefaultBuffer(resourceHandle, tempUpload);
}

void ModuleResources::createDefaultBuffer(ComPtr<ID3D12Resource>& resourceHandle, D3D12_RESOURCE_DESC& bufferDesc) {
	// Default buffer -> heap type = DEFAULT (this buffer will be read by the GPU)
	CD3DX12_HEAP_PROPERTIES vbHeapProps(D3D12_HEAP_TYPE_DEFAULT);
	device->CreateCommittedResource(&vbHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resourceHandle));
	app->getModuleD3D12().flush();
}

void ModuleResources::createDefaultBuffer(ComPtr<ID3D12Resource>& resourceHandle, D3D12_RESOURCE_DESC& bufferDesc, D3D12_RESOURCE_STATES resourceState) {
	// Default buffer -> heap type = DEFAULT (this buffer will be read by the GPU)
	CD3DX12_HEAP_PROPERTIES vbHeapProps(D3D12_HEAP_TYPE_DEFAULT);
	device->CreateCommittedResource(&vbHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, resourceState, nullptr, IID_PPV_ARGS(&resourceHandle));
	app->getModuleD3D12().flush();
}

void ModuleResources::createUploadBuffer(ComPtr<ID3D12Resource>& resourceHandle, const unsigned int bufferSize) {
	// Staging buffer -> heap type = UPLOAD (this buffer is used to upload data to the GPU)
	CD3DX12_HEAP_PROPERTIES sbHeapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC sbResDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
	device->CreateCommittedResource(&sbHeapProps, D3D12_HEAP_FLAG_NONE, &sbResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resourceHandle));
	app->getModuleD3D12().flush();
}

void ModuleResources::createUploadBufferWithData(ComPtr<ID3D12Resource>& resourceHandle, const void* data, const unsigned int bufferSize) {
	// Staging buffer -> heap type = UPLOAD (this buffer is used to upload data to the GPU)
	CD3DX12_HEAP_PROPERTIES sbHeapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC sbResDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
	device->CreateCommittedResource(&sbHeapProps, D3D12_HEAP_FLAG_NONE, &sbResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resourceHandle));
	copyDataToUploadBuffer(resourceHandle, data, bufferSize);
}

ComPtr<ID3D12Resource> ModuleResources::createUploadBuffer(const unsigned int bufferSize) {
	ComPtr<ID3D12Resource> resourceHandle;
	// Staging buffer -> heap type = UPLOAD (this buffer is used to upload data to the GPU)
	CD3DX12_HEAP_PROPERTIES sbHeapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC sbResDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
	device->CreateCommittedResource(&sbHeapProps, D3D12_HEAP_FLAG_NONE, &sbResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resourceHandle));
	app->getModuleD3D12().flush();
	return resourceHandle;
}

void ModuleResources::copyDataToUploadBuffer(ComPtr<ID3D12Resource>& resourceHandle, const void* data, const size_t size) {
	// Get a pointer to the resource in CPU (pData)
	BYTE* pData = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	HRESULT hr = resourceHandle.Get()->Map(0, &readRange, reinterpret_cast<void**>(&pData));

	// Copy the data from the CPU array to the Resource
	memcpy(pData, data, size);

	// Invalidates the pointer -> probably marks it as "used" ???
	resourceHandle.Get()->Unmap(0, nullptr);
}

void ModuleResources::copyDataFromUploadBufferToDefaultBuffer(ComPtr<ID3D12Resource>& defaultBufferHandle, ComPtr<ID3D12Resource>& uploadBufferHandle) {
	ID3D12GraphicsCommandList* copyCommandList = app->getModuleD3D12().getCopyCommandList().Get();
	ID3D12CommandAllocator* copyCommandAllocator = app->getModuleD3D12().getCopyCommandAllocator().Get();
	ID3D12CommandQueue* copyCommandQueue = app->getModuleD3D12().getCopyCommandQueue().Get();
	copyCommandList->Reset(copyCommandAllocator, nullptr);
	copyCommandList->CopyResource(defaultBufferHandle.Get(), uploadBufferHandle.Get());
 	copyCommandList->Close();
	ID3D12CommandList* lists[] = { copyCommandList };
	copyCommandQueue->ExecuteCommandLists(1, lists);
	app->getModuleD3D12().flush(copyCommandQueue);
}

ScratchImage ModuleResources::createTextureFromFile(const std::string& path, ComPtr<ID3D12Resource>& defaultBufferHandle, ComPtr<ID3D12Resource>& uploadBufferHandle) {
	DirectX::ScratchImage image;

	const std::string filePath = path;
	const std::wstring wstring = std::wstring(filePath.begin(), filePath.end());
	const wchar_t* wchar = wstring.c_str();

	bool ok = SUCCEEDED(DirectX::LoadFromDDSFile(wchar, DDS_FLAGS_NONE, nullptr, image));
	if (!ok)
		ok = SUCCEEDED(DirectX::LoadFromTGAFile(wchar, TGA_FLAGS_NONE, nullptr, image));
	if (!ok)
		ok = SUCCEEDED(DirectX::LoadFromWICFile(wchar, WIC_FLAGS_NONE, nullptr, image));
	if (!ok)
		return {};

	DirectX::TexMetadata metaData = image.GetMetadata();
	// Generate MipMaps if texture doesn't have any
	if (metaData.mipLevels == 1) {
		ScratchImage newImage;
		DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), TEX_FILTER_DEFAULT, 5, newImage);
		DirectX::TexMetadata newMetaData = newImage.GetMetadata();
		image = std::move(newImage);
		metaData = std::move(newMetaData);
	}

	D3D12_RESOURCE_DESC texBufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(metaData.format, UINT64(metaData.width),
		UINT(metaData.height), UINT16(metaData.arraySize),
		UINT16(metaData.mipLevels));

	app->getModuleResources().createDefaultBuffer(defaultBufferHandle, texBufferDesc);
	app->getModuleResources().createUploadBuffer(uploadBufferHandle, GetRequiredIntermediateSize(defaultBufferHandle.Get(), 0, image.GetImageCount()));

	std::vector<D3D12_SUBRESOURCE_DATA> subData;
	subData.reserve(image.GetImageCount());
	// Note we are iterating over mipLevels of each array item to respect Subresource index order
	for (size_t item = 0; item < metaData.arraySize; ++item)
	{
		for (size_t level = 0; level < metaData.mipLevels; ++level)
		{
			const DirectX::Image* subImg = image.GetImage(level, item, 0);
			D3D12_SUBRESOURCE_DATA data = { subImg->pixels, subImg->rowPitch, subImg->slicePitch };
			subData.push_back(data);
		}
	}

	// Need to UpdateSubresources using mipLevels * arraySize (total number of Subresources)
	UpdateSubresources(app->getModuleD3D12().getCurrentBufferCommandList().Get(), defaultBufferHandle.Get(), uploadBufferHandle.Get(), 0, 0, UINT(metaData.mipLevels * metaData.arraySize), subData.data());
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBufferHandle.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE);
	app->getModuleD3D12().getCurrentBufferCommandList()->ResourceBarrier(1, &barrier);


	app->getModuleShaderDescriptors().createGenericSRV(defaultBufferHandle.Get(), defaultBufferHandle->GetDesc().Format, defaultBufferHandle->GetDesc().MipLevels);

	return image;
}