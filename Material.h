#pragma once
#include "Globals.h"

class Texture;

class Material {
public:
	Material() {}
	~Material() {}

	Texture* getTexture() { return texture; }
	void setTexture(Texture* _texture) { texture = _texture; }

	float getMetallicFactor() { return metallicFactor; }
	void setMetallicFactor(float _metallicFactor) { metallicFactor = _metallicFactor; }

	Vector3 getEmissiveFactor() { return emissiveFactor; }
	void setEmissiveFactor(Vector3 _emissiveFactor) { emissiveFactor = _emissiveFactor; }

	std::string getName() { return name; }
	void setName(std::string _name) { name = _name; }
private:
	Texture* texture = nullptr;
	float metallicFactor = 0.0f;
	Vector3 emissiveFactor = { 0.0f, 0.0f, 0.0f };
	std::string name;
};