#include "Globals.h"
#include <fstream>
#include "Application.h"
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include "Utils.h"
#include "Mesh.h"
#include "TextureLoader.h"
#include "GameObject.h"
#include "ModuleBuffer.h"
#include "ModuleD3D12.h"
#include "Material.h"

#define ASSETS_RELATIVE_PATH "../"

extern Application* app;

bool Utils::loadGLTFAccessorData(uint8_t* data, size_t elemSize, size_t stride, size_t elemCount, const tinygltf::Model& model, const std::map<std::string, int> attributes, const char* attributeName) {

	const auto& it = attributes.find(attributeName);
	if (it != attributes.end()) {
		return loadGLTFAccessorData(data, elemSize, stride, elemCount, model, it->second);
	}

	return false;
}

bool Utils::loadGLTFAccessorData(uint8_t* data, size_t elemSize, size_t stride, size_t elemCount, const tinygltf::Model& model, int accessorIndex) {
	tinygltf::Accessor accessor = model.accessors.at(accessorIndex);
	tinygltf::BufferView bufferView = model.bufferViews.at(accessor.bufferView);
	tinygltf::Buffer buffer = model.buffers.at(bufferView.buffer);

	int32_t componentSizeInBytes = tinygltf::GetComponentSizeInBytes(accessor.componentType);
	int32_t numComponentsInType = tinygltf::GetNumComponentsInType(accessor.type);
	if (accessor.count != elemCount || elemSize != tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type))
		return false;

	if (stride == 0)
		stride = tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type);

	// Open and read from file
	std::ifstream file;

	// Get the buffer URI for this accessor
	std::string bufferURI = std::string("../") + buffer.uri;
	// Open in binary mode, default is text mode !!!
	file.open(bufferURI, std::ios::binary);
	if (!file.is_open()) {
		return false;
	}

	// Move the reading pointer to the current bufferView's 'byteOffset'
	file.seekg(accessor.byteOffset + bufferView.byteOffset, std::ios::beg);

	for (unsigned int i = 0; i < elemCount; i++) {
		file.read((char*)data, elemSize);
		data += stride;
	}

	file.close();
	return true;
}

bool Utils::loadMeshIntoGameObjectGLTF(const tinygltf::Model& model, unsigned int meshIndex, unsigned int primitiveIndex, GameObject* gameObject) {
	const tinygltf::Primitive primitive = model.meshes.at(meshIndex).primitives.at(primitiveIndex);
	const auto& itPos = primitive.attributes.find("POSITION");
	if (itPos != primitive.attributes.end()) {
		// Load vertices into Mesh
		uint32_t numVertices = uint32_t(model.accessors[itPos->second].count);
		Vertex* vertices = new Vertex[numVertices];
		uint8_t* vertexData = (uint8_t*)vertices; // Casts Vertex Buffer to Bytes (uint8_t*) buffer
		Utils::loadGLTFAccessorData(vertexData + offsetof(Vertex, position), sizeof(Vector3), sizeof(Vertex), numVertices, model, itPos->second);
		Utils::loadGLTFAccessorData(vertexData + offsetof(Vertex, normal), sizeof(Vector3), sizeof(Vertex), numVertices, model, primitive.attributes, "NORMAL");
		Utils::loadGLTFAccessorData(vertexData + offsetof(Vertex, texCoord), sizeof(Vector2), sizeof(Vertex), numVertices, model, primitive.attributes, "TEXCOORD_0");
		gameObject->getMesh()->setVertices(vertices);
		gameObject->getMesh()->setNumVertices(numVertices);

		// Load indices into Mesh
		uint32_t numIndices = model.accessors.at(primitive.indices).count;
		unsigned short* indices = new unsigned short[numIndices];
		uint8_t* indexData = (uint8_t*)indices; // Casts Vertex Buffer to Bytes (uint8_t*) buffer
		Utils::loadGLTFAccessorData(indexData, sizeof(unsigned short), sizeof(unsigned short), numIndices, model, primitive.indices);
		gameObject->getMesh()->setIndices(indices);
		gameObject->getMesh()->setNumIndices(numIndices);

		// Load material data
		Material* mat = new Material();
		float gltfMetallicFactor = model.materials.at(model.meshes.at(meshIndex).primitives.at(primitiveIndex).material).pbrMetallicRoughness.metallicFactor;
		std::vector<double> gltfEmissiveFactor = model.materials.at(model.meshes.at(meshIndex).primitives.at(primitiveIndex).material).emissiveFactor;
		mat->setName(model.materials.at(model.meshes.at(meshIndex).primitives.at(primitiveIndex).material).name);
		mat->setMetallicFactor(gltfMetallicFactor);
		mat->setEmissiveFactor(Vector3{ (float)gltfEmissiveFactor.at(0), (float)gltfEmissiveFactor.at(1), (float)gltfEmissiveFactor.at(2) });

		return true;
	}
	return false;
}

bool Utils::loadTextureIntoGameObjectGLTF(const tinygltf::Model& model, unsigned int meshIndex, unsigned int primitiveIndex, ComPtr<ID3D12Resource>& stagingTextureBuffer, ComPtr<ID3D12Resource>& gpuTextureBuffer) {
	unsigned int materialIndex = model.meshes.at(meshIndex).primitives.at(primitiveIndex).material;
	unsigned int textureIndex = model.materials.at(materialIndex).pbrMetallicRoughness.baseColorTexture.index;
	unsigned int textureImgIndex = model.textures.at(textureIndex).source;
	std::string textureImgURI = ASSETS_RELATIVE_PATH + model.images.at(textureImgIndex).uri;

	DirectX::ScratchImage image;
	TextureLoader::LoadFromFile(textureImgURI, image);

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

	app->getModuleBuffer()->createDefaultBuffer(gpuTextureBuffer, texBufferDesc);
	app->getModuleBuffer()->createUploadBuffer(stagingTextureBuffer, GetRequiredIntermediateSize(gpuTextureBuffer.Get(), 0, image.GetImageCount()));

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
	UpdateSubresources(app->getModuleD3D12()->getCurrentBufferCommandList().Get(), gpuTextureBuffer.Get(), stagingTextureBuffer.Get(), 0, 0, UINT(metaData.mipLevels * metaData.arraySize), subData.data());
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(gpuTextureBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE);
	app->getModuleD3D12()->getCurrentBufferCommandList()->ResourceBarrier(1, &barrier);

	D3D12_SHADER_RESOURCE_VIEW_DESC texSrvDesc = {};
	texSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	texSrvDesc.Format = gpuTextureBuffer->GetDesc().Format;
	texSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	texSrvDesc.Texture2D.MipLevels = gpuTextureBuffer->GetDesc().MipLevels;

	ComPtr<ID3D12Device2> device = app->getModuleD3D12()->getDevice();
	CD3DX12_CPU_DESCRIPTOR_HANDLE texCPUHandle(
		app->getModuleD3D12()->getShaderVisibleDescriptorHeap()->GetCPUDescriptorHandleForHeapStart(),
		0,
		device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
	);
	app->getModuleD3D12()->getDevice()->CreateShaderResourceView(gpuTextureBuffer.Get(), &texSrvDesc, texCPUHandle);
	return true;
}