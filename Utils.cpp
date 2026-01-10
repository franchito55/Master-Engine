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
#include "ModuleResources.h"
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

	DirectX::ScratchImage image = app->getModuleResources()->createTextureFromFile(textureImgURI, gpuTextureBuffer, stagingTextureBuffer);
	
	return true;
}