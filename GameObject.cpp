#include "Globals.h"
#include "GameObject.h"
#include "Mesh.h"
#include "Material.h"

GameObject::GameObject() {
	mesh = new Mesh();
	material = new Material();
}
GameObject::~GameObject() {
	// What to do? Multiple GameObjects might be pointing to same mesh/texture
	// delete mesh;
	// delete texture;
}