#pragma once
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext.hpp>
#include "../geometry/spatial/AABB.h"

struct Frustum {
	// In order of left, right, bottom, top, near, far
	glm::vec4 planes[6];

	bool containsOrIntersects(const AABB& aabb) const { 

		for (int i = 0; i < 6; i++) {
			if (!planeAABBIntersects(planes[i], aabb))
				return false;
		}

		return true; 

	}
	bool planeAABBIntersects(const glm::vec4& plane, const AABB& aabb) const {

		glm::vec4 c(aabb.getCenterPos(), 1.f);
		
		glm::vec3 h = aabb.getHalfSizes();
		float e = h.x*fabs(plane.x) + h.y*fabs(plane.y) + h.z*fabs(plane.z);
		float s = glm::dot(c, plane);

		if (s-e > 0)
			return false; // Outside

		// Else inside or intersecting
		return true;
	}
	void extractPlanes(const glm::mat4& vp) {

		planes[0].x = -(vp[0][3] + vp[0][0]);
		planes[0].y = -(vp[1][3] + vp[1][0]);
		planes[0].z = -(vp[2][3] + vp[2][0]);
		planes[0].w = -(vp[3][3] + vp[3][0]);

		planes[1].x = -(vp[0][3] - vp[0][0]);
		planes[1].y = -(vp[1][3] - vp[1][0]);
		planes[1].z = -(vp[2][3] - vp[2][0]);
		planes[1].w = -(vp[3][3] - vp[3][0]);

		planes[2].x = -(vp[0][3] + vp[0][1]);
		planes[2].y = -(vp[1][3] + vp[1][1]);
		planes[2].z = -(vp[2][3] + vp[2][1]);
		planes[2].w = -(vp[3][3] + vp[3][1]);

		planes[3].x = -(vp[0][3] - vp[0][1]);
		planes[3].y = -(vp[1][3] - vp[1][1]);
		planes[3].z = -(vp[2][3] - vp[2][1]);
		planes[3].w = -(vp[3][3] - vp[3][1]);

		planes[4].x = -(vp[0][2]);
		planes[4].y = -(vp[1][2]);
		planes[4].z = -(vp[2][2]);
		planes[4].w = -(vp[3][2]);

		planes[5].x = -(vp[0][3] - vp[0][2]);
		planes[5].y = -(vp[1][3] - vp[1][2]);
		planes[5].z = -(vp[2][3] - vp[2][2]);
		planes[5].w = -(vp[3][3] - vp[3][2]);
	}
};

class Camera {
	friend class CameraController;
public:

	Camera() {
		m_viewMatrix = glm::mat4(1.0f); // Identity matrix

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
			m_viewMatrix = glm::lookAtLH(m_pos, m_pos + m_direction, glm::vec3(0.f, 1.f, 0.f));
			m_updateVPMatrix = true;
			m_updateViewMatrix = false;
		}

		if (m_updateVPMatrix) {
			m_VPMatrix = getProjectionMatrix() * m_viewMatrix;
			// Update frustum planes
			m_frustum.extractPlanes(m_VPMatrix);
			m_updateVPMatrix = false;
		}

		return m_VPMatrix;
	}

	const glm::mat4& getViewMatrix() {
		if (m_updateViewMatrix) {
			m_viewMatrix = glm::lookAtLH(m_pos, m_pos + m_direction, glm::vec3(0.f, 1.f, 0.f));
			m_updateVPMatrix = true;
			m_updateViewMatrix = false;
		}
		return m_viewMatrix;
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

protected:
	glm::mat4 m_VPMatrix;

	glm::mat4 m_viewMatrix;

private:
	virtual const glm::mat4& getProjectionMatrix() = 0;

private:
	bool m_updateVPMatrix, m_updateViewMatrix;
	bool m_updateUp;

	glm::vec3 m_pos;
	glm::vec3 m_direction;
	glm::vec3 m_up;

	Frustum m_frustum;

};
