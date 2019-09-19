#pragma once

#include "pch.h" // for SNAPSHOT_BUFFER_IND

class Transform {

public:
	// To be done at the end of each CPU update and nowhere else	
	static void IncrementCurrentUpdateIndex();

	// To be done just before render is called
	static void UpdateCurrentRenderIndex();

#ifdef _DEBUG
	static UINT GetUpdateIndex();
	static UINT GetRenderIndex();
#endif

	explicit Transform(Transform* parent);
	Transform(const glm::vec3& translation, Transform* parent = nullptr);
	Transform(const glm::vec3& translation = { 0.0f, 0.0f, 0.0f }, const glm::vec3& rotation = { 0.0f, 0.0f, 0.0f }, const glm::vec3& scale = { 1.0f, 1.0f, 1.0f }, Transform* parent = nullptr);
	virtual ~Transform();

	void setParent(Transform* parent);
	void removeParent();

	void copyDataFromPrevUpdate();

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

	void setMatrix(const glm::mat4& newMatrix);


	const glm::vec3& getTranslation() const;
	const glm::vec3& getRotations() const;
	const glm::vec3& getScale() const;

	glm::mat4 getMatrix(float alpha = 1.0f);
	glm::mat4 getLocalMatrix();

private:
	struct TransformSnapshot {
		glm::vec3 m_translation;
		glm::vec3 m_rotation;
		glm::vec3 m_scale;

		bool m_matNeedsUpdate;
		bool m_parentUpdated;
	};
	TransformSnapshot m_transformSnapshots[SNAPSHOT_BUFFER_SIZE];

	glm::mat4 m_transformMatrix;
	glm::mat4 m_localTransformMatrix;

	Transform* m_parent;
	std::vector<Transform*> m_children;


private:
	void updateLocalMatrix();
	void updateMatrix();
	void treeNeedsUpdating();
	void addChild(Transform* transform);
	void removeChild(Transform* transform);


	static constexpr int prevInd(int ind) {
		return (ind + SNAPSHOT_BUFFER_SIZE - 1) % SNAPSHOT_BUFFER_SIZE;
	}

	// first frame is 0 and it continues from there, integer overflow isn't a problem unless
	// you leave the game running for like a year or two.
	// Note: atomic since it's written to in every update and read from in every update and render
	static std::atomic_uint s_frameIndex;
	
	// the index in the snapshot buffer that is used in the update loop on the CPU.
	// [0, SNAPSHOT_BUFFER_SIZE-1]
	// Note: Updated once at the start of update and read-only in update so no atomics needed
	static UINT s_updateIndex;
	
	
	// If CPU update is working on index 3 then prepare render will safely interpolate between
	// index 1 and 2 without any data races
	// Note: Updated once at the start of render and read-only in render so no atomics needed
	static UINT s_renderIndex;
};