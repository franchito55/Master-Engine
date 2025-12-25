#pragma once
#include "Globals.h"
#include <cstdint>
#include <map>
#include <string>
#include "3rdParty/tinygltf/tiny_gltf.h"

class Mesh;
class GameObject;

namespace tinygltf {
	class Model;
}

class Utils {
public: 
	static bool loadGLTFAccessorData(uint8_t* data, size_t elemSize, size_t stride, size_t elemCount, const tinygltf::Model& model, const std::map<std::string, int> attributes, const char* attributeName);
	static bool loadGLTFAccessorData(uint8_t* data, size_t elemSize, size_t stride, size_t elemCount, const tinygltf::Model& model, int accessorIndex);
	static bool loadMeshIntoGameObjectGLTF(const tinygltf::Model& model, unsigned int meshIndex, unsigned int primitiveIndex, GameObject* gameObject);
	static bool loadTextureIntoGameObjectGLTF(const tinygltf::Model& model, unsigned int meshIndex, unsigned int primitiveIndex, ComPtr<ID3D12Resource>& stagingTextureBuffer, ComPtr<ID3D12Resource>& gpuTextureBuffer);
};