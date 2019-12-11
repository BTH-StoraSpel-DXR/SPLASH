#include "pch.h"
#include "Animation.h"

#pragma region FRAME

Animation::Frame::Frame() :
	m_transformSize(0),
	m_limbTransform(nullptr){
}
Animation::Frame::Frame(glm::mat4* limbTransform, const unsigned int size) {
	m_limbTransform = limbTransform;
	m_transformSize = size;
}
Animation::Frame::Frame(const unsigned int size) :
	m_transformSize(size) {
	m_limbTransform = SAIL_NEW glm::mat4[size];
	for (unsigned int i = 0; i < m_transformSize; i++) {
		m_limbTransform[i] = glm::identity<glm::mat4>();
	}
}
Animation::Frame::~Frame() {
	Memory::SafeDeleteArr(m_limbTransform);
}
void Animation::Frame::setTransform(const unsigned int index, const glm::mat4& transform) {
#ifdef _DEBUG
	if (index >= m_transformSize || index < 0) {
		#if defined(_DEBUG) && defined(SAIL_VERBOSELOGGING)
		SAIL_LOG_ERROR("Tried to add transform to index(" + std::to_string(index) + ") maxSize(" + std::to_string(m_transformSize));
		#endif
		return;
	}
#endif
	m_limbTransform[index] = transform;

}
const unsigned int Animation::Frame::getTransformListSize() {
	return m_transformSize;
}
const glm::mat4* Animation::Frame::getTransformList() {
	return m_limbTransform;
}

unsigned int Animation::Frame::getByteSize() const {
	unsigned int size = 0;

	size += sizeof(*this);
	size += sizeof(glm::mat4) * m_transformSize;
	
	return size;
}

#pragma endregion

#pragma region ANIMATION

Animation::Animation() :
	m_maxFrame(0) ,
	m_maxFrameTime(0),
	m_name("")
{

}
Animation::Animation(const std::string& name) : Animation() {
	m_name = name;
}
Animation::~Animation() {
	for (unsigned int frame = 0; frame <= m_maxFrame; frame++) {
		Memory::SafeDelete(m_frames[frame]);
	}
}
const float Animation::getMaxAnimationTime() {
	if (exists(m_maxFrame)) {
		return m_frameTimes[m_maxFrame];
	}
	return 0.0f;
}
const unsigned int Animation::getMaxAnimationFrame() {
	return m_maxFrame;
}

const glm::mat4* Animation::getAnimationTransform(const float time, const FindType type) {
	const unsigned int frame = getFrameAtTime(time, type);
	return getAnimationTransform(frame);
}
const glm::mat4* Animation::getAnimationTransform(const unsigned int frame) {
	if (exists(frame)) {
		return m_frames[frame]->getTransformList();
	}
	return nullptr;
}

const unsigned int Animation::getAnimationTransformSize(const float time) {
	const unsigned int frame = getFrameAtTime(time);
	return getAnimationTransformSize(frame);
}
const unsigned int Animation::getAnimationTransformSize(const unsigned int frame) {
	if (exists(frame)) {
		return m_frames[frame]->getTransformListSize();
	}
	return 0;
}

const float Animation::getTimeAtFrame(const unsigned int frame) {
	if (exists(frame)) {
		return m_frameTimes[frame];
	}
	return 0.0f;
}
const unsigned int Animation::getFrameAtTime(float time, const FindType type) {
	time = fmodf(time, getMaxAnimationTime());

	float leastDiff = 50000;
	unsigned int closestFrame = m_maxFrame;
	/* find closest frame */
	for (unsigned int frame = 0; frame < m_maxFrame; frame++) {
		float diff = fabsf(m_frameTimes[frame] - time);
		if (diff < leastDiff) {
			leastDiff = diff;
			closestFrame = frame;
		} else {
			continue;
		}
	}

	if (closestFrame != m_maxFrame) {
		switch (type) {
		case BEHIND:
			if (closestFrame == 0) {
				return m_maxFrame - 1;
			} else {
				return closestFrame - 1;
			}
			break;
		case INFRONT:
			if (closestFrame == m_maxFrame - 1) {
				return 0;
			} else {
				return closestFrame + 1;
			}
			break;
		default:
			return closestFrame;
			break;
		}
	} else {
		// Couldn't find a frame for some reason?
		SAIL_LOG_WARNING("Animation::getFrameAtTime: Couldn't find a proper frame.");
		return 0;
	}
}

void Animation::addFrame(const unsigned int frame, const float time, Animation::Frame* data) {
	if (m_maxFrameTime < time) {
		m_maxFrameTime = time;
	}
	if (m_maxFrame < frame) {
		m_maxFrame = frame;
	}

	m_frames[frame] = data;
	m_frameTimes[frame] = time;
}

void Animation::setName(const std::string& name) {
	m_name = name;
}
const std::string& Animation::getName() {
	return m_name;
}

unsigned int Animation::getByteSize() const {
	unsigned int size = 0;
	size += sizeof(*this);

	size += m_name.capacity() * sizeof(unsigned char);
	
	size += sizeof(std::pair<unsigned int, float>) * m_frameTimes.size();

	size += sizeof(std::pair<unsigned int, Animation::Frame*>) * m_frames.size();

	for (auto& [key, val] : m_frames) {
		size += val->getByteSize();
	}

	return size;
}

inline const bool Animation::exists(const unsigned int frame) {
	if (m_frames.find(frame) == m_frames.end()) {
		#if defined(_DEBUG) && defined(SAIL_VERBOSELOGGING)
			SAIL_LOG_WARNING("Trying to access frame(" + std::to_string(frame) + ") which does not exist, maxFrame(" + std::to_string(m_maxFrame) + ").");
		#endif
		return false;
	}
	return true;
}

#pragma endregion

#pragma region VERTCONNECTION

AnimationStack::VertConnection::VertConnection() :
	count(0)
{
	for (unsigned int i = 0; i < SAIL_BONES_PER_VERTEX; i++) {
		transform[i] = 0;
		weight[i] = 0.0f;
	}
}

void AnimationStack::VertConnection::addConnection(const unsigned int _transform, const float _weight) {
	if (count >= SAIL_BONES_PER_VERTEX) {
		#if defined(_DEBUG) && defined(SAIL_VERBOSELOGGING)
			SAIL_LOG_ERROR("AnimationStack:VertConnection: Too many existing connections(" + std::to_string(count) + ")");
		#endif
		return;
	}
	transform[count] = _transform;
	weight[count] = _weight;
	count++;
}

const float AnimationStack::VertConnection::checkWeights() {
	float sum = 0;
	for (unsigned int i = 0; i < count; i++) {
		sum += weight[i];
	}
	return sum;
}

void AnimationStack::VertConnection::normalizeWeights() {
	float sum = 0.0f;
	for (unsigned int weightIndex = 0; weightIndex < count; weightIndex++) {
		sum += weight[weightIndex];
	}
	for (unsigned int weightIndex = 0; weightIndex < count; weightIndex++) {
		weight[weightIndex] /= sum;
	}
}



#pragma endregion

#pragma region ANIMATIONSTACK

AnimationStack::AnimationStack() {
	m_connectionSize = 0;
	m_connections = nullptr;
}
AnimationStack::AnimationStack(const unsigned int vertCount) : AnimationStack(){
	m_connectionSize = vertCount;
	m_connections = SAIL_NEW VertConnection[vertCount];

}
AnimationStack::~AnimationStack() {
	for (unsigned int index = 0; index < m_stack.size(); index++) {
		Memory::SafeDelete(m_stack[m_names[index]]);
	}
	Memory::SafeDeleteArr(m_connections);
}
void AnimationStack::reSizeConnections(const unsigned int vertCount) {
	VertConnection* temp = SAIL_NEW VertConnection[vertCount];
	for (unsigned int i = 0; i < m_connectionSize; i++) {
		temp[i] = m_connections[i];
	}
	Memory::SafeDeleteArr(m_connections);
	m_connectionSize = vertCount;
	m_connections = temp;

}
void AnimationStack::addAnimation(const std::string& animationName, Animation* animation) {
	if (m_stack.find(animationName) == m_stack.end()) {
		m_names[(unsigned int)m_stack.size()] = animationName;
		m_indexes[animationName] = (unsigned int)m_stack.size();
		m_stack[animationName] = animation;
	}
	else {
#if defined(_DEBUG) && defined(SAIL_VERBOSELOGGING)
		SAIL_LOG_WARNING("Replacing animation " + animationName);
#endif
		m_stack[animationName] = animation;
	}
}
void AnimationStack::setConnectionData(const unsigned int vertexIndex, const unsigned int boneIndex, float weight) {
#ifdef _DEBUG
	if (vertexIndex > m_connectionSize) {
#if defined(_DEBUG) && defined(SAIL_VERBOSELOGGING)
		SAIL_LOG_ERROR("AnimationStack::setBoneData: vertexIndex("+ std::to_string(vertexIndex) +") larger than array size("+std::to_string(m_connectionSize) +")." );
#endif
		return;
	}
#endif
	if (m_connections[vertexIndex].count >= SAIL_BONES_PER_VERTEX) {
#if defined(_DEBUG) && defined(SAIL_VERBOSELOGGING)
		SAIL_LOG_ERROR("AnimationStack::SetConnectionData: vertexIndex: " + std::to_string(vertexIndex) + "");
#endif
	}
	m_connections[vertexIndex].addConnection(boneIndex, weight);
}

void AnimationStack::addBone(AnimationStack::Bone& bone) {
	m_bones.push_back(bone);
}
const unsigned int AnimationStack::boneCount() {
	return m_bones.size();
}
AnimationStack::Bone& AnimationStack::getBone(const unsigned int index) {
	return m_bones[index];
}

unsigned int AnimationStack::getByteSize() {
	unsigned int size = 0;
	
	size += sizeof(*this);
	
	size += sizeof(VertConnection) * m_connectionSize;

	size += sizeof(std::pair<unsigned int, std::string>) * m_names.size();
	for (auto& [key, val] : m_names) {
		size += val.capacity() * sizeof(unsigned char);
	}

	size += sizeof(std::pair<std::string, unsigned int>) * m_indexes.size();
	for (auto& [key, val] : m_indexes) {
		size += key.capacity() * sizeof(unsigned char);
	}

	size += sizeof(std::pair<std::string, Animation*>) * m_stack.size();
	for (auto& [key, val] : m_stack) {
		size += key.capacity() * sizeof(unsigned char);

		size += val->getByteSize();
	}

	for (auto& bone : m_bones) {
		size += bone.getByteSize();
	}

	return size;
}

Animation* AnimationStack::getAnimation(const std::string& name) {
	if (m_stack.find(name) == m_stack.end()) {
		return nullptr;
	}
	return m_stack[name];
}
Animation* AnimationStack::getAnimation(const unsigned int index) {
	if (m_names.find(index) == m_names.end())
		return nullptr;
	if (m_stack.find(m_names[index]) == m_stack.end())
		return nullptr;

	return m_stack[m_names[index]];
}


const unsigned int AnimationStack::getAnimationIndex(const std::string& name) {
	if (m_indexes.find(name) == m_indexes.end()) {
		return 0;
	}
	else {
		return m_indexes[name];
	}
}
const std::string AnimationStack::getAnimationName(const unsigned int index) {
	if (m_names.find(index) == m_names.end()) {
		return "";
	}
	else {
		return m_names[index];
	}
}
const unsigned int AnimationStack::getAnimationCount() {
	return (unsigned int)m_stack.size();
}

const glm::mat4* AnimationStack::getTransform(const std::string& name, const float time) {
	Animation* animation = getAnimation(name);
	if (!animation)
		return nullptr;
	return animation->getAnimationTransform(time);
}
const glm::mat4* AnimationStack::getTransform(const std::string& name, const unsigned int frame) {
	Animation* animation = getAnimation(name);
	if (!animation)
		return nullptr;
	return animation->getAnimationTransform(frame);
}
const glm::mat4* AnimationStack::getTransform(const unsigned int index, const float time) {
	Animation* animation = getAnimation(index);
	if (!animation)
		return nullptr;
	return animation->getAnimationTransform(time);
}
const glm::mat4* AnimationStack::getTransform(const unsigned int index, const unsigned int frame) {
	Animation* animation = getAnimation(index);
	if (!animation)
		return nullptr;
	return animation->getAnimationTransform(frame);
}

AnimationStack::VertConnection* AnimationStack::getConnections() {
	return m_connections;
}

void AnimationStack::setConnections(VertConnection* con, unsigned int size) {
	m_connections = con;
	m_connectionSize = size;
}

const unsigned int AnimationStack::getConnectionSize() {
	return m_connectionSize;
}

void AnimationStack::checkWeights() {

	for (unsigned int i = 0; i < m_connectionSize; i++) {
		if (m_connections[i].count == 0) {
#if defined(_DEBUG) && defined(SAIL_VERBOSELOGGING)
			SAIL_LOG_WARNING("count == 0: " + std::to_string(i));
#endif
		}

		float value = m_connections[i].checkWeights();
		if (value > 1.001 || value < 0.999) {
#if defined(_DEBUG) && defined(SAIL_VERBOSELOGGING)
			SAIL_LOG_WARNING("Weights fucked: " + std::to_string(i) + "(" + std::to_string(value) + ")");
#endif
		}
	}
}

void AnimationStack::normalizeWeights() {

	for (unsigned int i = 0; i < m_connectionSize; i++) {
		m_connections[i].normalizeWeights();
	}

}

#pragma endregion

