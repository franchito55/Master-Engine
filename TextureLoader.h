#pragma once
#include "Globals.h"
#include "DirectXTex.h"

class TextureLoader {
public:
	static void LoadFromDDSFile(const wchar_t* szFile, ScratchImage& image);
};