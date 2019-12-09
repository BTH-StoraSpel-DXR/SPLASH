#pragma once
#include <d3d11.h>
#include <glm/glm.hpp>

class PointLight {
public:
	PointLight() : m_color(glm::vec3(0.f)), m_position(glm::vec3(0.f)), m_radius(10.f), m_index(-1) { }
	void setColor(const glm::vec3& color) { m_color = color; }
	const glm::vec3& getColor() const { return m_color; }
	void setPosition(const glm::vec3& position) { m_position = position; }
	const glm::vec3& getPosition() const { return m_position; }
	void setRadius(float radius) { m_radius = radius; }
	float getRadius() const { return m_radius; }
	void setIndex(size_t index) {
		m_index = index;
	}
	size_t getIndex() const { 
		return m_index; 
	}

private:
	glm::vec3 m_color;
	glm::vec3 m_position;
	size_t m_index;
	float m_radius;
};

class SpotLight : public PointLight {
	
public:
	SpotLight() : PointLight(), m_direction(glm::vec3(1.0f, 0.0f, 0.0f)), m_angle(0.5) {}
	void setDirection(const glm::vec3& dir) { m_direction = dir; }
	const glm::vec3& getDirection() const { return m_direction; }

	void setAngle(const float angle) { m_angle = angle; }
	float getAngle() const { return m_angle; }

private:
	glm::vec3 m_direction;
	float m_angle;
};