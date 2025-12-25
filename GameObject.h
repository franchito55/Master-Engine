#pragma once
#include "Globals.h"
#include "./structs/Transform.h"

class Mesh;
class Material;

class GameObject {
public:
	GameObject();
	~GameObject();
	Mesh* getMesh() { return mesh; }
	void setMesh(Mesh* _mesh) { mesh = _mesh; }
	Material* getMaterial() { return material; }
	void setMaterial(Material* _material) { material = _material; }
	Transform getTransform() { return transform; }
	void setTransform(Transform _transform) { transform = _transform; }
private:
	Mesh* mesh = nullptr;
	Material* material = nullptr;
	Transform transform;
};