#include "Globals.h"
#include "TextureLoader.h"

bool TextureLoader::LoadFromFile(const std::string& szFile, ScratchImage& image) {
	const std::wstring wstring = std::wstring(szFile.begin(), szFile.end());
	const wchar_t* wchar = wstring.c_str();

	DirectX::LoadFromDDSFile(wchar, DDS_FLAGS_NONE, nullptr, image);
	if (image.GetImageCount() > 0)
		return true;

	DirectX::LoadFromTGAFile(wchar, TGA_FLAGS_NONE, nullptr, image);
	if (image.GetImageCount() > 0)
		return true;

	DirectX::LoadFromWICFile(wchar, WIC_FLAGS_NONE, nullptr, image);
	if (image.GetImageCount() != 0)
		return true;

	return false;
}