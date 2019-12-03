#include "pch.h"
#include "TextureData.h"

TextureData::TextureData() {
	m_data.channels = 4;
	m_data.height = 0;
	m_data.width = 0;
	m_data.textureData = nullptr;
}
TextureData::TextureData(const std::string& filename) {
	load(filename);
}
TextureData::~TextureData() {
	Memory::SafeDeleteArr(m_data.textureData);
}

void TextureData::load(const std::string& filename) {
	FileLoader::TGALoader TGALoader(filename, m_data);
}

unsigned int TextureData::getWidth() const {
	return m_data.width;
}
unsigned int TextureData::getHeight() const {
	return m_data.height;
}

unsigned int TextureData::getBytesPerPixel() const {
	// TODO: change bitsPerChannel to match the loaded image
	unsigned int bitsPerChannel = 8;
	return (m_data.channels * bitsPerChannel) / 8;
}

unsigned char* TextureData::getTextureData() const {
	return m_data.textureData;
}
glm::vec4 TextureData::getPixel(unsigned int x, unsigned int y) {

	if (x < 0 || x > m_data.width - 1) return glm::vec4(0.f);
	if (y < 0 || y > m_data.height - 1) return glm::vec4(0.f);

	return glm::vec4(	m_data.textureData[y * m_data.width * m_data.channels + (x * m_data.channels)],
					m_data.textureData[y * m_data.width * m_data.channels + (x * m_data.channels) + 1],
					m_data.textureData[y * m_data.width * m_data.channels + (x * m_data.channels) + 2],
					m_data.textureData[y * m_data.width * m_data.channels + (x * m_data.channels) + 3]);

}

unsigned int  TextureData::getByteSize() const {
	return sizeof(*this) + sizeof(unsigned char) * m_data.width * m_data.height * m_data.channels;
}
