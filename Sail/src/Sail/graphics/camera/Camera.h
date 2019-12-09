#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "Frustum.h"

class Camera {
	friend class CameraController;
public:
	Camera() {
		viewMatrix = glm::mat4(1.0f); // Identity matrix

		m_pos = glm::vec3(0.f, 0.f, 0.f);
		m_direction = glm::vec3(0.f, 0.f, -1.f);
		m_up = glm::vec3(0.f, 1.f, 0.f);

		m_updateVPMatrix = true;
		m_updateViewMatrix = true;
		m_updateUp = true;
	}

	void lookAt(const glm::vec3& pos) {
		m_direction = pos - m_pos;
		m_direction = glm::normalize(m_direction);
		m_updateViewMatrix = true;
	}
	void setPosition(const glm::vec3& pos) {
		m_pos = pos;
		m_updateViewMatrix = true;
	}
	const glm::vec3& getPosition() const {
		return m_pos;
	}
	void setDirection(const glm::vec3& dir) {
		m_direction = dir;
		m_updateViewMatrix = true;
		m_updateUp = true;
	}
	const glm::vec3& getDirection() const {
		return m_direction;
	}
	

	const glm::mat4& getViewProjection() {
		if (m_updateViewMatrix) {
			viewMatrix = glm::lookAtLH(m_pos, m_pos + m_direction, glm::vec3(0.f, 1.f, 0.f));
			m_updateVPMatrix = true;
			m_updateViewMatrix = false;
		}

		if (m_updateVPMatrix) {
			vpMatrix = getProjectionMatrix() * viewMatrix;
			// Update frustum planes
			m_frustum.extractPlanes(vpMatrix);
			m_updateVPMatrix = false;
		}

		return vpMatrix;
	}

	const glm::mat4& getViewMatrix() {
		if (m_updateViewMatrix) {
			viewMatrix = glm::lookAtLH(m_pos, m_pos + m_direction, glm::vec3(0.f, 1.f, 0.f));
			m_updateVPMatrix = true;
			m_updateViewMatrix = false;
		}
		return viewMatrix;
	}
	const glm::mat4& getProjMatrix() {
		return getProjectionMatrix();
	}

	const glm::vec3& getUp() {
		if (m_updateUp) {
			glm::vec3 right = glm::cross(glm::vec3(0.f, 1.f, 0.f), m_direction);
			m_up = glm::cross(m_direction, right);
			m_up = glm::normalize(m_up);
			m_updateUp = false;
		}
		return m_up;
	}

	const Frustum& getFrustum() {
		// Update frustum planes to the latest if VP matrix needs to be updated
		if (m_updateViewMatrix || m_updateVPMatrix)
			m_frustum.extractPlanes(getViewProjection());
		return m_frustum;
	}

	float getNearZ() const {
		return nearZDst;
	}
	float getFarZ() const {
		return farZDst;
	}

	void newFrame() {
		// Store last used vp matrix
		m_viewMatrixLastFrame = getViewMatrix();
		m_projMatrixLastFrame = getProjectionMatrix();
	}

	glm::mat4 getViewProjectionLastFrame() const {
		return m_projMatrixLastFrame * m_viewMatrixLastFrame;
	}

private:
	virtual const glm::mat4& getProjectionMatrix() = 0;

protected:
	glm::mat4 vpMatrix;
	glm::mat4 viewMatrix;


	float nearZDst;
	float farZDst;

private:
	bool m_updateVPMatrix, m_updateViewMatrix;
	bool m_updateUp;
	glm::mat4 m_viewMatrixLastFrame;
	glm::mat4 m_projMatrixLastFrame;

	glm::vec3 m_pos;
	glm::vec3 m_direction;
	glm::vec3 m_up;

	Frustum m_frustum;

};
