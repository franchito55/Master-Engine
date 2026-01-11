#pragma once
#include "Globals.h"

struct Vertex {
	Vector3 position;
	Vector3 normal;
	Vector2 texCoord;
};

class Mesh {
private:
	Vertex* vertices = nullptr;
	unsigned int numVertices = 0;
	unsigned short* indices = nullptr;
	unsigned int numIndices = 0;
	unsigned int materialIndex = 0;

public:
	Vertex* getVertices() { return vertices; }
	void setVertices(Vertex* _vertices) { vertices = _vertices; }
	unsigned int getNumVertices() { return numVertices; }
	void setNumVertices(unsigned int _numVertices) { numVertices = _numVertices; }

	unsigned short* getIndices() { return indices; }
	void setIndices(unsigned short* _indices) { indices = _indices; }
	unsigned int getNumIndices() { return numIndices; }
	void setNumIndices(unsigned int _numIndices) { numIndices = _numIndices; }
};