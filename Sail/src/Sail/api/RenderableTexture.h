#pragma once

//#include <glm/glm.hpp>
#include <glm/vec4.hpp>

class RenderableTexture {
public:
	RenderableTexture(unsigned int width = 320, unsigned int height = 180);
	virtual ~RenderableTexture();

	virtual void begin() = 0;
	virtual void end() = 0;
	virtual void clear(const glm::vec4& color) = 0;
	virtual void resize(int width, int height) = 0;

protected:

private:

};