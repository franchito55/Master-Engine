#pragma once
#include "Globals.h"
#include "DirectXTex.h"
#include <iostream>

class TextureLoader {
public:
	static bool LoadFromFile(const std::string& szFile, ScratchImage& image);
};