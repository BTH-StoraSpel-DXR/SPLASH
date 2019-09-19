#include "pch.h"
#include "Transform.h"

std::atomic_uint Transform::s_frameIndex = 0;
UINT Transform::s_updateIndex = 0;
UINT Transform::s_renderIndex = 0;

// STATIC FUNCTIONS

// To be done at the end of each CPU update and nowhere else	
void Transform::IncrementCurrentUpdateIndex() {
	s_frameIndex++;
	s_updateIndex = s_frameIndex.load() % SNAPSHOT_BUFFER_SIZE;
}

// To be done just before render is called
void Transform::UpdateCurrentRenderIndex() {
	s_renderIndex = prevInd(s_frameIndex.load());
}

#ifdef _DEBUG
UINT Transform::GetUpdateIndex() { return s_updateIndex; }
UINT Transform::GetRenderIndex() { return s_renderIndex; }
#endif

// NON-STATIC FUNCTIONS

Transform::Transform(Transform* parent)
	: Transform::Transform({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, parent) 
{}

Transform::Transform(const glm::vec3& translation, Transform* parent)
	: Transform(translation, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, parent) 
{}

Transform::Transform(const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale, Transform* parent)
	: m_transformMatrix(1.0f)
	, m_localTransformMatrix(1.0f)
	, m_parent(parent) 
{
	for (auto& ts : m_transformSnapshots) {
		ts.m_translation = translation;
		ts.m_rotation = rotation;
		ts.m_scale = scale;
		ts.m_matNeedsUpdate = true;
		ts.m_parentUpdated = parent;
	}

	if (m_parent)
		m_parent->addChild(this);
}

Transform::~Transform() {}

void Transform::setParent(Transform* parent) {
	if (m_parent) {
		m_parent->removeChild(this);
	}
	m_parent = parent;
	parent->addChild(this);
	for (auto& ts : m_transformSnapshots) {
		ts.m_parentUpdated = true;
	}
	for (int i = 0; i < SNAPSHOT_BUFFER_SIZE; i++) {
		treeNeedsUpdating();
	}
}

void Transform::removeParent() {
	if (m_parent) {
		m_parent->removeChild(this);
		m_parent = nullptr;
	}
}

// NOTE: Has to be done at the beginning of each update
void Transform::copyDataFromPrevUpdate() {
	m_transformSnapshots[s_updateIndex] = m_transformSnapshots[prevInd(s_updateIndex)];
}

void Transform::translate(const glm::vec3& move) {
	m_transformSnapshots[s_updateIndex].m_translation += move;
	m_transformSnapshots[s_updateIndex].m_matNeedsUpdate = true;
	treeNeedsUpdating();
}

void Transform::setStartTranslation(const glm::vec3& translation) {
	for (auto& ts : m_transformSnapshots) {
		ts.m_translation = translation;
		ts.m_matNeedsUpdate = true;
	}
}

void Transform::translate(const float x, const float y, const float z) {
	m_transformSnapshots[s_updateIndex].m_translation += glm::vec3(x, y, z);
	m_transformSnapshots[s_updateIndex].m_matNeedsUpdate = true;
	treeNeedsUpdating();
}

void Transform::scale(const float factor) {
	m_transformSnapshots[s_updateIndex].m_scale *= factor;
	m_transformSnapshots[s_updateIndex].m_matNeedsUpdate = true;
	treeNeedsUpdating();
}

void Transform::scale(const glm::vec3& scale) {
	m_transformSnapshots[s_updateIndex].m_scale *= scale;
	m_transformSnapshots[s_updateIndex].m_matNeedsUpdate = true;
	treeNeedsUpdating();
}

void Transform::rotate(const glm::vec3& rotation) {
	m_transformSnapshots[s_updateIndex].m_rotation += rotation;
	m_transformSnapshots[s_updateIndex].m_matNeedsUpdate = true;
	treeNeedsUpdating();
}

void Transform::rotate(const float x, const float y, const float z) {
	m_transformSnapshots[s_updateIndex].m_rotation += glm::vec3(x, y, z);
	m_transformSnapshots[s_updateIndex].m_matNeedsUpdate = true;
	treeNeedsUpdating();
}

void Transform::rotateAroundX(const float radians) {
	m_transformSnapshots[s_updateIndex].m_rotation.x += radians;
	m_transformSnapshots[s_updateIndex].m_matNeedsUpdate = true;
	treeNeedsUpdating();
}

void Transform::rotateAroundY(const float radians) {
	m_transformSnapshots[s_updateIndex].m_rotation.y += radians;
	m_transformSnapshots[s_updateIndex].m_matNeedsUpdate = true;
	treeNeedsUpdating();
}

void Transform::rotateAroundZ(const float radians) {
	m_transformSnapshots[s_updateIndex].m_rotation.z += radians;
	m_transformSnapshots[s_updateIndex].m_matNeedsUpdate = true;
	treeNeedsUpdating();
}

void Transform::setTranslation(const glm::vec3& translation) {
	m_transformSnapshots[s_updateIndex].m_translation = translation;
	m_transformSnapshots[s_updateIndex].m_matNeedsUpdate = true;
	treeNeedsUpdating();
}

void Transform::setTranslation(const float x, const float y, const float z) {
	m_transformSnapshots[s_updateIndex].m_translation = glm::vec3(x, y, z);
	m_transformSnapshots[s_updateIndex].m_matNeedsUpdate = true;
	treeNeedsUpdating();
}

void Transform::setRotations(const glm::vec3& rotations) {
	m_transformSnapshots[s_updateIndex].m_rotation = rotations;
	m_transformSnapshots[s_updateIndex].m_matNeedsUpdate = true;
	treeNeedsUpdating();
}

void Transform::setRotations(const float x, const float y, const float z) {
	m_transformSnapshots[s_updateIndex].m_rotation = glm::vec3(x, y, z);
	m_transformSnapshots[s_updateIndex].m_matNeedsUpdate = true;
	treeNeedsUpdating();
}

void Transform::setScale(const float scale) {
	m_transformSnapshots[s_updateIndex].m_scale = glm::vec3(scale, scale, scale);
	m_transformSnapshots[s_updateIndex].m_matNeedsUpdate = true;
	treeNeedsUpdating();
}

void Transform::setScale(const float x, const float y, const float z) {
	m_transformSnapshots[s_updateIndex].m_scale = glm::vec3(x, y, z);
	m_transformSnapshots[s_updateIndex].m_matNeedsUpdate = true;
	treeNeedsUpdating();
}

void Transform::setScale(const glm::vec3& scale) {
	m_transformSnapshots[s_updateIndex].m_scale = scale;
	m_transformSnapshots[s_updateIndex].m_matNeedsUpdate = true;
	treeNeedsUpdating();
}

// NOTE: Not used anywhere at the moment
void Transform::setMatrix(const glm::mat4& newMatrix) {
	m_localTransformMatrix = newMatrix;
	glm::vec3 tempSkew;
	glm::vec4 tempPerspective;
	glm::quat tempRotation;
	glm::decompose(newMatrix, 
		m_transformSnapshots[s_updateIndex].m_scale, 
		tempRotation, 
		m_transformSnapshots[s_updateIndex].m_translation, 
		tempSkew, tempPerspective);
	// TODO: Check that rotation is valid
	m_transformSnapshots[s_updateIndex].m_rotation = glm::eulerAngles(tempRotation);

	m_transformSnapshots[s_updateIndex].m_matNeedsUpdate = false;
	treeNeedsUpdating();
}


// Note: returns the translation/rotation/scale that's currently used in update
const glm::vec3& Transform::getTranslation() const {
	return m_transformSnapshots[s_updateIndex].m_translation;
}
const glm::vec3& Transform::getRotations() const {
	return m_transformSnapshots[s_updateIndex].m_rotation;
}
const glm::vec3& Transform::getScale() const {
	return m_transformSnapshots[s_updateIndex].m_scale;
}

// TODO: use alpha to interpolate between transform snapshots
// alpha = [0,1], a value of 1 is the most recent snapshot and 0 is the one before that
// Used by render
glm::mat4 Transform::getMatrix(const float alpha) {
	if (m_transformSnapshots[s_renderIndex].m_matNeedsUpdate) {
		updateLocalMatrix();
		m_transformSnapshots[s_renderIndex].m_matNeedsUpdate = false;
	}
	if (m_transformSnapshots[s_renderIndex].m_parentUpdated || !m_parent) {
		updateMatrix();
		m_transformSnapshots[s_renderIndex].m_parentUpdated = false;
	}

	return m_transformMatrix;
}

// Not used anywhere
glm::mat4 Transform::getLocalMatrix() {
	if (m_transformSnapshots[s_renderIndex].m_matNeedsUpdate) {
		updateLocalMatrix();
		m_transformSnapshots[s_renderIndex].m_matNeedsUpdate = false;
	}
	return m_localTransformMatrix;
}

void Transform::updateLocalMatrix() {
	m_localTransformMatrix = glm::mat4(1.0f);
	m_localTransformMatrix = glm::translate(m_localTransformMatrix, m_transformSnapshots[s_renderIndex].m_translation);
	m_localTransformMatrix = glm::rotate(m_localTransformMatrix, m_transformSnapshots[s_renderIndex].m_rotation.x, glm::vec3(1.f, 0.f, 0.f));
	m_localTransformMatrix = glm::rotate(m_localTransformMatrix, m_transformSnapshots[s_renderIndex].m_rotation.y, glm::vec3(0.f, 1.f, 0.f));
	m_localTransformMatrix = glm::rotate(m_localTransformMatrix, m_transformSnapshots[s_renderIndex].m_rotation.z, glm::vec3(0.f, 0.f, 1.f));
	m_localTransformMatrix = glm::scale(m_localTransformMatrix, m_transformSnapshots[s_renderIndex].m_scale);
}

void Transform::updateMatrix() {
	if (m_parent)
		m_transformMatrix = m_parent->getMatrix() * m_localTransformMatrix;
	else
		m_transformMatrix = m_localTransformMatrix;
}

void Transform::treeNeedsUpdating() {
	m_transformSnapshots[s_updateIndex].m_parentUpdated = true;
	for (Transform* child : m_children) {
		child->treeNeedsUpdating();
	}
}

void Transform::addChild(Transform* transform) {
	m_children.push_back(transform);
}

void Transform::removeChild(Transform* transform) {
	for (int i = 0; i < m_children.size(); i++) {
		if (m_children[i] == transform) {
			m_children[i] = m_children.back();
			m_children.pop_back();
			break;
		}
	}
}

const bool Transform::getChange() {
	bool tempChange = m_hasChanged;
	m_hasChanged = false;
	return tempChange;
}