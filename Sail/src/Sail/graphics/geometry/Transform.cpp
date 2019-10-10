#include "pch.h"
#include "Transform.h"

Transform::Transform(Transform* parent)
	: Transform::Transform({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, parent) 
{}

Transform::Transform(const glm::vec3& translation, Transform* parent)
	: Transform(translation, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, parent) 
{}

Transform::Transform(const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale, Transform* parent)
	: m_parent(parent) 
{
	m_data.m_current.m_translation = translation;
	m_data.m_current.m_rotation = rotation;
	m_data.m_current.m_rotationQuat = rotation;
	m_data.m_current.m_scale = scale;
	m_data.m_current.m_forward = glm::vec3(0.0f);
	m_data.m_current.m_right = glm::vec3(0.0f);
	m_data.m_current.m_up = glm::vec3(0.0f);

	m_data.m_previous.m_translation = translation;
	m_data.m_previous.m_rotation = rotation;
	m_data.m_previous.m_rotationQuat = rotation;
	m_data.m_previous.m_scale = scale;
	m_data.m_previous.m_forward = glm::vec3(0.0f);
	m_data.m_previous.m_right = glm::vec3(0.0f);
	m_data.m_previous.m_up = glm::vec3(0.0f);

	m_matNeedsUpdate = true;
	m_parentUpdated = parent;
	m_parentRenderUpdated = parent;
	m_hasChanged |= 2;

	if (m_parent)
		m_parent->addChild(this);
}

Transform::~Transform() {
	removeParent();
	removeChildren();
}

void Transform::setParent(Transform* parent) {
	if (m_parent) {
		m_parent->removeChild(this);
	}
	m_parent = parent;
	parent->addChild(this);
	m_parentUpdated = true;
	treeNeedsUpdating();
}

void Transform::removeParent() {
	if (m_parent) {
		m_parent->removeChild(this);
		m_parent = nullptr;
	}
}

// NOTE: Has to be done at the beginning of each update
// Call from PrepareUpdateSystem and nowhere else!
void Transform::prepareUpdate() {
	m_data.m_previous = m_data.m_current;
	m_hasChanged = 0;
}


TransformSnapshot Transform::getCurrentTransformState() const {
	return m_data.m_current;
}

TransformSnapshot Transform::getPreviousTransformState() const {
	return m_data.m_previous;
}

TransformFrame Transform::getTransformFrame() const {
	return m_data;
}

void Transform::translate(const glm::vec3& move) {
	m_data.m_current.m_translation += move;
	m_matNeedsUpdate = true;
	m_hasChanged |= 1;
	treeNeedsUpdating();
}

void Transform::setStartTranslation(const glm::vec3& translation) {
	m_data.m_previous.m_translation = translation;
	m_data.m_current.m_translation = translation;
	m_matNeedsUpdate = true;
	m_hasChanged |= 1;
}

void Transform::translate(const float x, const float y, const float z) {
	m_data.m_current.m_translation += glm::vec3(x, y, z);
	m_matNeedsUpdate = true;
	m_hasChanged |= 1;
	treeNeedsUpdating();
}

void Transform::scale(const float factor) {
	m_data.m_current.m_scale *= factor;
	m_matNeedsUpdate = true;
	m_hasChanged |= 2;
	treeNeedsUpdating();
}

void Transform::scale(const glm::vec3& scale) {
	m_data.m_current.m_scale *= scale;
	m_matNeedsUpdate = true;
	m_hasChanged |= 2;
	treeNeedsUpdating();
}

void Transform::rotate(const glm::vec3& rotation) {
	m_data.m_current.m_rotation += rotation;
	m_data.m_current.m_rotationQuat = glm::quat(m_data.m_current.m_rotation);
	m_matNeedsUpdate = true;
	m_hasChanged |= 2;
	treeNeedsUpdating();
}

void Transform::rotate(const float x, const float y, const float z) {
	m_data.m_current.m_rotation += glm::vec3(x, y, z);
	m_data.m_current.m_rotationQuat = glm::quat(m_data.m_current.m_rotation);
	m_matNeedsUpdate |= 2;
	treeNeedsUpdating();
}

void Transform::rotateAroundX(const float radians) {
	m_data.m_current.m_rotation.x += radians;
	m_data.m_current.m_rotationQuat = glm::quat(m_data.m_current.m_rotation);
	m_matNeedsUpdate = true;
	m_hasChanged |= 2;
	treeNeedsUpdating();
}

void Transform::rotateAroundY(const float radians) {
	m_data.m_current.m_rotation.y += radians;
	m_data.m_current.m_rotationQuat = glm::quat(m_data.m_current.m_rotation);
	m_matNeedsUpdate = true;
	m_hasChanged |= 2;
	treeNeedsUpdating();
}

void Transform::rotateAroundZ(const float radians) {
	m_data.m_current.m_rotation.z += radians;
	m_data.m_current.m_rotationQuat = glm::quat(m_data.m_current.m_rotation);
	m_matNeedsUpdate = true;
	m_hasChanged |= 2;
	treeNeedsUpdating();
}

void Transform::setTranslation(const glm::vec3& translation) {
	m_data.m_current.m_translation = translation;
	m_matNeedsUpdate = true;
	m_hasChanged |= 1;
	treeNeedsUpdating();
}

void Transform::setTranslation(const float x, const float y, const float z) {
	m_data.m_current.m_translation = glm::vec3(x, y, z);
	m_matNeedsUpdate = true;
	m_hasChanged |= 1;
	treeNeedsUpdating();
}

void Transform::setRotations(const glm::vec3& rotations) {
	m_data.m_current.m_rotation = rotations;
	m_data.m_current.m_rotationQuat = glm::quat(m_data.m_current.m_rotation);
	m_matNeedsUpdate = true;
	m_hasChanged |= 2;
	treeNeedsUpdating();
}

void Transform::setRotations(const float x, const float y, const float z) {
	m_data.m_current.m_rotation = glm::vec3(x, y, z);
	m_data.m_current.m_rotationQuat = glm::quat(m_data.m_current.m_rotation);
	m_matNeedsUpdate = true;
	m_hasChanged |= 2;
	treeNeedsUpdating();
}

void Transform::setScale(const float scale) {
	m_data.m_current.m_scale = glm::vec3(scale, scale, scale	);
	m_matNeedsUpdate = true;
	m_hasChanged |= 2;
	treeNeedsUpdating();
}

void Transform::setScale(const float x, const float y, const float z) {
	m_data.m_current.m_scale = glm::vec3(x, y, z	);
	m_matNeedsUpdate = true;
	m_hasChanged |= 2;
	treeNeedsUpdating();
}

void Transform::setScale(const glm::vec3& scale) {
	m_data.m_current.m_scale = scale;
	m_matNeedsUpdate = true;
	m_hasChanged |= 2;
	treeNeedsUpdating();
}

void Transform::setForward(const glm::vec3& forward) {
	m_data.m_current.m_forward = glm::vec3(forward.x, forward.y, -forward.z);
	m_data.m_current.m_rotationQuat = glm::rotation(glm::vec3(0.f, 0.f, -1.f), m_data.m_current.m_forward);
	m_matNeedsUpdate = true;
	m_hasChanged |= 2;
}


Transform* Transform::getParent() const {
	return m_parent;
}

const glm::vec3& Transform::getTranslation() const {
	return m_data.m_current.m_translation;
}
const glm::vec3& Transform::getRotations() const {
	return m_data.m_current.m_rotation;
}
const glm::vec3& Transform::getScale() const {
	return m_data.m_current.m_scale;
}

const glm::vec3 Transform::getInterpolatedTranslation(float alpha) const {
	return (alpha * m_data.m_current.m_translation) + ((1.0f - alpha) * m_data.m_previous.m_translation);
}

const glm::quat Transform::getInterpolatedRotation(float alpha) const {
	return (alpha * m_data.m_current.m_rotationQuat) + ((1.0f - alpha) * m_data.m_previous.m_rotationQuat);
}


glm::mat4 Transform::getMatrix() {
	if (m_matNeedsUpdate) {
		updateLocalMatrix();
		m_matNeedsUpdate = false;
	}
	if (m_parentUpdated || !m_parent) {
		updateMatrix();
		m_parentUpdated = false;
	}

	return m_transformMatrix;
}

glm::mat4 Transform::getRenderMatrix(float alpha) {
	// If data hasn't changed use the CPU side transformMatrix
	if (!m_hasChanged && !m_parentRenderUpdated) {
		m_renderMatrix = getMatrix();
	} else {
		// if the data has changed between updates then the matrix will be interpolated every frame
		updateLocalRenderMatrix(alpha);

		updateRenderMatrix(alpha); // if it has then interpolate
		if (m_parentRenderUpdated || !m_parent) {
			updateRenderMatrix(alpha);
			m_parentRenderUpdated = false;
		}
	}

	return m_renderMatrix;
}

void Transform::updateLocalRenderMatrix(float alpha) {
	// Linear interpolation between the two most recent snapshots
	glm::vec3 trans = (alpha * m_data.m_current.m_translation) + ((1.0f - alpha) * m_data.m_previous.m_translation);
	glm::quat rot = (alpha * m_data.m_current.m_rotationQuat) + ((1.0f - alpha) * m_data.m_previous.m_rotationQuat);
	glm::vec3 scale = (alpha * m_data.m_current.m_scale) + (1.0f - alpha) * m_data.m_previous.m_scale;

	createTransformMatrix(m_localRenderMatrix, trans, rot, scale);
}

void Transform::updateRenderMatrix(float alpha) {
	if (m_parent) {
		m_renderMatrix = m_parent->getRenderMatrix(alpha) * m_localRenderMatrix;
	} else {
		m_renderMatrix = m_localRenderMatrix;
	}
}

void Transform::updateLocalMatrix() {
	createTransformMatrix(m_localTransformMatrix, m_data.m_current.m_translation, m_data.m_current.m_rotationQuat, m_data.m_current.m_scale);
}

void Transform::updateMatrix() {
	if (m_parent) {
		m_transformMatrix = m_parent->getMatrix() * m_localTransformMatrix;
	} else {
		m_transformMatrix = m_localTransformMatrix;
	}
}

void Transform::treeNeedsUpdating() {
	m_parentUpdated = true;
	m_parentRenderUpdated = true;
	if (m_parent) {
		m_hasChanged = m_parent->getChange() | m_hasChanged;
	}
	for (Transform* child : m_children) {
		child->treeNeedsUpdating();
	}
}

void Transform::addChild(Transform* transform) {
	m_children.push_back(transform);
}

void Transform::removeChild(Transform* Transform) {
	for (int i = 0; i < m_children.size(); i++) {
		if (m_children[i] == Transform) {
			m_children[i] = m_children.back();
			m_children.pop_back();
			break;
		}
	}
}

void Transform::removeChildren() {
	for ( auto child : m_children ) {
		child->removeParent();
	}
	m_children.clear();
}

void Transform::createTransformMatrix(glm::mat4& destination, const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale) const {
	glm::mat4 prev = glm::mat4(destination);
	destination = glm::mat4_cast(rotation);
	
	// Column major means destination[0] is the first column, not the first row
	destination[0].x *= scale.x;
	destination[0].y *= scale.x;
	destination[0].z *= scale.x;

	destination[1].x *= scale.y;
	destination[1].y *= scale.y;
	destination[1].z *= scale.y;

	destination[2].x *= scale.z;
	destination[2].y *= scale.z;
	destination[2].z *= scale.z;

	// Apply translation
	destination[3].x = translation.x;
	destination[3].y = translation.y;
	destination[3].z = translation.z;
}

const int Transform::getChange() {
	return m_hasChanged;
}