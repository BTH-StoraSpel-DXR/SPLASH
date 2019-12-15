#pragma once

#include <d3d11.h>
#include <string>
#include "loaders/TGALoader.h"
#include "ResourceFormat.h"

class TextureData {
public:
	TextureData();
	TextureData(const std::string& filename);
	~TextureData();

	void load(const std::string& filename);

	unsigned int getWidth() const;
	unsigned int getHeight() const;
	unsigned int getBytesPerPixel() const;
	unsigned char* getTextureData() const;
	glm::vec4 getPixel(unsigned int x, unsigned int y);
	
	unsigned int getByteSize() const;

	const std::string& getFileName() const;

private:
	ResourceFormat::TextureData m_data;
	std::string m_fileName;
};