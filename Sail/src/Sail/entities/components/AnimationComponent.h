#pragma once

#include "Component.h"
#include "Sail/graphics/geometry/Animation.h"
#include "Sail/api/Mesh.h"
#include "Sail/api/VertexBuffer.h"
#include <queue>
#include "Sail/entities/Entity.h"

class AnimationComponent : public Component<AnimationComponent> {
public:
	//SAIL_COMPONENT 
	AnimationComponent(AnimationStack* animationStack) :
		animationTime(0),
		animationIndex(0),
		animationSpeed(1.0f),
		animationName(""),
		currentAnimation(nullptr),
		nextAnimation(nullptr),
		currentTransition(nullptr),
		blending(false),
		dataSize(0),
		transformSize(0),
		computeUpdate(true),
		animationW(0.0f), //TODO: REMOVE
		m_stack(animationStack),
		rightHandEntity(nullptr),
		leftHandEntity(nullptr)

	{
		transformSize = m_stack->getAnimation(0)->getAnimationTransformSize(unsigned int(0));
		transforms = SAIL_NEW glm::mat4[transformSize];
	}
	



	~AnimationComponent() {

		Memory::SafeDeleteArr(data.indices);
		Memory::SafeDeleteArr(data.positions);
		Memory::SafeDeleteArr(data.normals);
		Memory::SafeDeleteArr(data.bitangents);
		Memory::SafeDeleteArr(data.colors);
		Memory::SafeDeleteArr(data.tangents);
		Memory::SafeDeleteArr(data.texCoords);
		Memory::SafeDeleteArr(transforms);

	}
	unsigned int dataSize;
	

	bool computeUpdate;
	float animationTime;
	unsigned int animationIndex;
	float animationSpeed;
	std::string animationName;
	Animation* currentAnimation;
	Animation* nextAnimation;
	bool blending;
	unsigned int transformSize;
	bool hasUpdated;
	float animationW;	
	
	// -0.55, 1.03, 0.11
	glm::mat4 rightHandPosition;
	// 0.57, 1.03, 0.11
	glm::mat4 leftHandPosition;

	glm::vec3 lPos;
	glm::vec3 rPos;
	
	Entity* rightHandEntity;
	Entity* leftHandEntity;


	class Transition {
	public:
		Transition(Animation* _to, const float time = 1.0f, const bool wait = true) {
			to = _to;
			transitionTime = time;
			transpiredTime = 0.0f;
			waitForEnd = wait;
		}
		Animation* to;
		float transitionTime;
		float transpiredTime;
		bool waitForEnd;
	};
	std::queue<Transition> transitions;
	Transition* currentTransition;

	Mesh::Data data;
	glm::mat4* transforms;

	std::unique_ptr<VertexBuffer> tposeVBuffer;

	AnimationStack* getAnimationStack() { return m_stack; };
private:
	AnimationStack* m_stack;
};