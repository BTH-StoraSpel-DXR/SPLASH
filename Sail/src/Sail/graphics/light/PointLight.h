#pragma once
#include <glm/glm.hpp>

class PointLight {
public:
	struct Attenuation {
		float constant;
		float linear;
		float quadratic;
	};
public:
	PointLight() : m_color(glm::vec3(0.f)), m_position(glm::vec3(0.f)), m_attenuation({ 1.f, 1.f, 1.f }), m_index(-1) { }
	void setColor(const glm::vec3& color) { m_color = color; }
	const glm::vec3& getColor() const { return m_color; }
	void setPosition(const glm::vec3& position) { m_position = position; }
	const glm::vec3& getPosition() const { return m_position; }
	void setAttenuation(float constant, float linear, float quadratic) {
		m_attenuation.constant = constant;
		m_attenuation.linear = linear;
		m_attenuation.quadratic = quadratic;
		calculateRadius();
	}
	const Attenuation& getAttenuation() const { return m_attenuation; }
	float getRadius() const { return m_radius; }
	void setIndex(const int index) {
		m_index = index;
	}
	 int getIndex()const { return m_index; }
private:
	void calculateRadius() {
		// Derived from attenuation formula used in light shader
		m_radius = (-m_attenuation.linear + std::sqrt(std::pow(m_attenuation.linear, 2.f) - 4.f * m_attenuation.quadratic*(m_attenuation.constant - 100))) / (2 * m_attenuation.quadratic);
	}
private:
	glm::vec3 m_color;
	glm::vec3 m_position;
	Attenuation m_attenuation;
	int m_index;
	float m_radius;
};