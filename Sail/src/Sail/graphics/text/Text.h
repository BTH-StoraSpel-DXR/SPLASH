#pragma once

//#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <memory>
//#include <SpriteBatch.h>

class SailFont;

class Text {
public:
	typedef std::unique_ptr<Text> Ptr;
	static Ptr Create(const std::wstring& text, const glm::vec2& pos = glm::vec2(0.f), const glm::vec4& color = glm::vec4(1.0f));
public:
	Text(const std::wstring& text, const glm::vec2& pos = glm::vec2(0.f), const glm::vec4& color = glm::vec4(1.f));

	void setText(const std::wstring& text);
	const std::wstring& getText() const;
	void setPosition(const glm::vec2& pos);
	const glm::vec2& getPosition() const;
	void setColor(const glm::vec4& color);
	const glm::vec4& getColor() const;

private:
	std::wstring m_text;
	glm::vec2 m_pos;
	glm::vec4 m_color;

};