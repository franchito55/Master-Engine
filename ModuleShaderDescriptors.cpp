#include "Globals.h"
#include "ModuleShaderDescriptors.h"
#include "Application.h"
#include "ModuleD3D12.h"

#define SHADER_VISIBLE_DESCRIPTOR_NUMBER 1000
#define MAX_SAMPLERS 10

extern Application* app;

bool ModuleShaderDescriptors::init() {
	app->setModuleShaderDescriptors(this);
	device = app->getModuleD3D12()->getDevice();
	genericDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	samplerDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

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
	app->getModuleD3D12()->getDevice()->CreateDescriptorHeap(&shaderVisibleDescriptorHeapDesc, IID_PPV_ARGS(&genericSrvHeap));

	// ============ Init descriptor heap for samplers ============
	D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
	// Type = Sampler
	samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	// How many RTVs (2 -> 1 back buffer, 1 front buffer)
	samplerHeapDesc.NumDescriptors = MAX_SAMPLERS;
	// Shader-visible ?
	samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	// Flags for multi-adapter. Which adapter this descriptor is for
	samplerHeapDesc.NodeMask = 0;
	app->getModuleD3D12()->getDevice()->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&samplerHeap));

	return true;
}

unsigned int ModuleShaderDescriptors::createGenericSRV(ID3D12Resource* resource, DXGI_FORMAT format, unsigned int mipLevels) {
	// 1. Reserve (?) next free slot
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = mipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	unsigned int index = nextFreeGenericIndex++;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle = getCPUHandleFromGenericHeap(index);

	device->CreateShaderResourceView(resource, &srvDesc, cpuDescriptorHandle);
	
	return index;
}

unsigned int ModuleShaderDescriptors::createSampler(D3D12_SAMPLER_DESC* samplerDesc) {
	unsigned int index = nextFreeSamplerIndex++;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = getCPUHandleFromSamplerHeap(index);
	device->CreateSampler(samplerDesc, cpuHandle);
	return index;
}

unsigned int ModuleShaderDescriptors::allocateDescriptor() {
	// 1. Reserve (?) next free slot
	// 2. Update next free index to +1
	return 0;
}

void ModuleShaderDescriptors::createDefaultSamplers() {
	D3D12_SAMPLER_DESC samplers[] = {
		// 1. Linear/Wrap Sampler
		{
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, // Linear filtering for min, mag, and mip
			D3D12_TEXTURE_ADDRESS_MODE_WRAP, // Wrap addressing mode
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			0.0f, // Mip LOD bias
			1, // Max anisotropy
			D3D12_COMPARISON_FUNC_ALWAYS, // No comparison
			{ 0, 0, 0, 0 }, // Border color (not used)
			0.0f, D3D12_FLOAT32_MAX // Min and Max LOD
		},
		// 2. Point/Wrap Sampler
		{
			D3D12_FILTER_MIN_MAG_MIP_POINT, // Linear filtering for min, mag, and mip
			D3D12_TEXTURE_ADDRESS_MODE_WRAP, // Wrap addressing mode
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			0.0f, // Mip LOD bias
			1, // Max anisotropy
			D3D12_COMPARISON_FUNC_ALWAYS, // No comparison
			{ 0, 0, 0, 0 }, // Border color (not used)
			0.0f, D3D12_FLOAT32_MAX // Min and Max LOD
		},
		// 3. Linear/Clamp Sampler
		{
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, // Linear filtering for min, mag, and mip
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // Wrap addressing mode
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			0.0f, // Mip LOD bias
			1, // Max anisotropy
			D3D12_COMPARISON_FUNC_ALWAYS, // No comparison
			{ 0, 0, 0, 0 }, // Border color (not used)
			0.0f, D3D12_FLOAT32_MAX // Min and Max LOD
		},
		// 4. Point/Clamp Sampler
		{
			D3D12_FILTER_MIN_MAG_MIP_POINT, // Linear filtering for min, mag, and mip
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // Wrap addressing mode
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			0.0f, // Mip LOD bias
			1, // Max anisotropy
			D3D12_COMPARISON_FUNC_ALWAYS, // No comparison
			{ 0, 0, 0, 0 }, // Border color (not used)
			0.0f, D3D12_FLOAT32_MAX // Min and Max LOD
		}
	};

	for (unsigned int i = 0; i < std::size(samplers); i++) {
		createSampler(&samplers[i]);
	}
}