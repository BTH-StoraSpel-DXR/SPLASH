#pragma once
#include <glm/vec3.hpp>


#define PI 3.14159265359
#define PI_2 6.28318530718

// Structs for storing transform data from two consecutive updates
// so that they can be interpolated between.
// Should be optimized more in the future.
struct TransformSnapshot {
	glm::vec3 m_translation;
	glm::vec3 m_rotation;
	glm::quat m_rotationQuat;
	glm::vec3 m_scale;
	glm::vec3 m_forward;
	glm::vec3 m_right;
	glm::vec3 m_up;

};

struct TransformFrame {
	TransformSnapshot m_current;
	TransformSnapshot m_previous;

	bool m_updatedDirections;
};

class Transform {

public:
	explicit Transform(Transform* parent);
	Transform(const glm::vec3& translation, Transform* parent = nullptr);
	Transform(const glm::vec3& translation = { 0.0f, 0.0f, 0.0f },
		const glm::vec3& rotation = { 0.0f, 0.0f, 0.0f },
		const glm::vec3& scale = { 1.0f, 1.0f, 1.0f },
		Transform* parent = nullptr);
	virtual ~Transform();

	void setParent(Transform* parent);
	void removeParent();

	void prepareUpdate();
	TransformSnapshot getCurrentTransformState() const;
	TransformSnapshot getPreviousTransformState() const;

	TransformFrame getTransformFrame() const;

	void setStartTranslation(const glm::vec3& translation);

	void translate(const glm::vec3& move);
	void translate(const float x, const float y, const float z);

	void scale(const float factor);
	void scale(const glm::vec3& scale);

	//In radians
	void rotate(const glm::vec3& rotation);
	//In radians
	void rotate(const float x, const float y, const float z);
	void rotateAroundX(const float radians);
	void rotateAroundY(const float radians);
	void rotateAroundZ(const float radians);


	void setTranslation(const glm::vec3& translation);
	void setTranslation(const float x, const float y, const float z);

	void setRotations(const glm::vec3& rotations);
	void setRotations(const float x, const float y, const float z);
	void setScale(const float scale);
	void setScale(const float x, const float y, const float z);
	void setScale(const glm::vec3& scale);

	/* Forward should always be a normalized vector */
	void setForward(const glm::vec3& forward);

	Transform* getParent() const;

	const glm::vec3& getTranslation() const;
	const glm::vec3& getRotations() const;
	const glm::vec3& getScale() const;

	const glm::vec3 getInterpolatedTranslation(float alpha) const;
	const glm::quat getInterpolatedRotation(float alpha) const;

	// Matrix used by collision etc.
	glm::mat4 getMatrix();

	// Matrix used to render
	glm::mat4 getRenderMatrix(float alpha = 1.0f);

private:
	TransformFrame m_data;

	// Used for collision detection
	// At most updated once per tick
	glm::mat4 m_transformMatrix;
	glm::mat4 m_localTransformMatrix;

	// Used for rendering
	// At most updated once per frame
	glm::mat4 m_renderMatrix;
	glm::mat4 m_localRenderMatrix;

	bool m_parentUpdated;
	bool m_parentRenderUpdated;
	int m_hasChanged;     // If the data has been changed since the last update
	bool m_matNeedsUpdate; // Will only be false if m_hasChanged == false and a matrix has been created

	Transform* m_parent = nullptr;

	std::vector<Transform*> m_children;
private:
	void updateLocalMatrix();
	void updateMatrix();

	void updateLocalRenderMatrix(float alpha);
	void updateRenderMatrix(float alpha);

	void treeNeedsUpdating();
	void addChild(Transform* transform);
	void removeChild(Transform* transform);
	void removeChildren();

	// Modifies the elements of matrix directly instead of multiplying with matrices
	void createTransformMatrix(glm::mat4& destination, const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale) const;

private:
	friend class UpdateBoundingBoxSystem;
	const int getChange(); //Only access this from UpdateBoundingBoxSystem::update()
};