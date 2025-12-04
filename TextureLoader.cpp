#include "Globals.h"
#include "TextureLoader.h"

void TextureLoader::LoadFromDDSFile(const wchar_t* szFile, ScratchImage& image) {
	DirectX::LoadFromDDSFile(szFile, DDS_FLAGS_NONE, nullptr, image);
}