#include "pch.h"
#include "Texture.h"
#include "Sail/Application.h"

TextureData& Texture::getTextureData(const std::string& filename) const {
	// Load the texture file it if is not loaded already
	auto& rm = Application::getInstance()->getResourceManager();
	if (!rm.hasTextureData(filename)) {
		rm.loadTextureData(filename);
	}
	return rm.getTextureData(filename);
}
