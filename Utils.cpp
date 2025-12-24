#include "Globals.h"
#include <fstream>
#include "Application.h"
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include "Utils.h"
#include "Mesh.h"

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

bool Utils::loadIntoMesh(const tinygltf::Model& model, Mesh& mesh, const tinygltf::Primitive& primitive) {
	const auto& itPos = primitive.attributes.find("POSITION");
	if (itPos != primitive.attributes.end()) {
		// Load vertices into Mesh
		uint32_t numVertices = uint32_t(model.accessors[itPos->second].count);
		Vertex* vertices = new Vertex[numVertices];
		uint8_t* vertexData = (uint8_t*)vertices; // Casts Vertex Buffer to Bytes (uint8_t*) buffer
		Utils::loadGLTFAccessorData(vertexData + offsetof(Vertex, position), sizeof(Vector3), sizeof(Vertex), numVertices, model, itPos->second);
		Utils::loadGLTFAccessorData(vertexData + offsetof(Vertex, normal), sizeof(Vector3), sizeof(Vertex), numVertices, model, primitive.attributes, "NORMAL");
		Utils::loadGLTFAccessorData(vertexData + offsetof(Vertex, texCoord), sizeof(Vector2), sizeof(Vertex), numVertices, model, primitive.attributes, "TEXCOORD_0");
		mesh.setVertices(vertices);
		mesh.setNumVertices(numVertices);

		// Load indices into Mesh
		uint32_t numIndices = model.accessors.at(primitive.indices).count;
		unsigned short* indices = new unsigned short[numIndices];
		uint8_t* indexData = (uint8_t*)indices; // Casts Vertex Buffer to Bytes (uint8_t*) buffer
		Utils::loadGLTFAccessorData(indexData, sizeof(unsigned short), sizeof(unsigned short), numIndices, model, primitive.indices);
		mesh.setIndices(indices);
		mesh.setNumIndices(numIndices);

		return true;
	}
	return false;
}