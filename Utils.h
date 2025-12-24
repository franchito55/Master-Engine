#pragma once
#include "Globals.h"
#include <cstdint>
#include <map>
#include <string>
#include "3rdParty/tinygltf/tiny_gltf.h"

class Mesh;

namespace tinygltf {
	class Model;
}

class Utils {
public: 
	static bool loadGLTFAccessorData(uint8_t* data, size_t elemSize, size_t stride, size_t elemCount, const tinygltf::Model& model, const std::map<std::string, int> attributes, const char* attributeName);
	static bool loadGLTFAccessorData(uint8_t* data, size_t elemSize, size_t stride, size_t elemCount, const tinygltf::Model& model, int accessorIndex);
	static bool loadIntoMesh(const tinygltf::Model& model, Mesh& mesh, const tinygltf::Primitive& primitive);
};