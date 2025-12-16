#pragma once
#include <vector>

class Mesh {
private:
	std::vector<float> vertices = {};
	std::vector<unsigned int> indices = {};
	unsigned int materialIndex = 0;
};