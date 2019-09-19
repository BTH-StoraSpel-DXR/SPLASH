#pragma once

#include <glm/glm.hpp>

class RenderableTexture {
public:
	static RenderableTexture* Create(unsigned int width = 320, unsigned int height = 180, const std::string& name = "Renderable Texture");
	virtual ~RenderableTexture() {}

	virtual void begin(void* cmdList = nullptr) = 0;
	virtual void end(void* cmdList = nullptr) = 0;
	virtual void clear(const glm::vec4& color, void* cmdList = nullptr) = 0;
	virtual void resize(int width, int height) = 0;

protected:

private:

};