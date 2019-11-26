#pragma once

#include "Component.h"
#include "Sail/graphics/geometry/Animation.h"
#include "Sail/api/Mesh.h"
#include "Sail/api/VertexBuffer.h"
#include <queue>
#include "Sail/entities/Entity.h"

/*
	Skeleton joints indices

	0  : Hips
	1  : Spine
	2  : Spine1
	3  : Spine2
	4  : Neck
	5  : Head
	6  : HeadTop_End
	7  : LeftShoulder
	8  : LeftArm
	9  : LeftForeArm
	10 : LeftHand
	11 : LeftHandThumb1
	12 : LeftHandThumb2
	13 : LeftHandThumb3
	14 : LeftHandThumb4
	15 : LeftHandIndex1
	16 : LeftHandIndex2
	17 : LeftHandIndex3
	18 : LeftHandIndex4
	19 : RightShoulder
	20 : RightArm
	21 : RightForeArm
	22 : RightHand
	23 : RightHandThumb1
	24 : RightHandThumb2
	25 : RightHandThumb3
	26 : RightHandThumb4
	27 : RightHandIndex1
	28 : RightHandIndex2
	29 : RightHandIndex3
	30 : RightHandIndex4
	31 : LeftUpLeg
	32 : LeftLeg
	33 : LeftFoot
	34 : LeftToeBase
	35 : LeftToe_End
	36 : RightUpLeg
	37 : RightLeg
	38 : RightFoot
	39 : RightToeBase
	40 : RightToe_End
*/


class AnimationComponent : public Component<AnimationComponent> {
public:
	class Transition {
	public:
		Transition() {
			to = nullptr;
			transitionTime = 1.0f;
			transpiredTime = 0.0f;
			waitForEnd = true;
			done = true;
			toIndex = 51020130;
		}
		Transition(Animation* _to, const float time = 1.0f, const bool wait = true) {
			to = _to;
			transitionTime = time;
			transpiredTime = 0.0f;
			waitForEnd = wait;
			done = true;
			toIndex = 51020130;
		}
		Animation* to;
		float transitionTime;
		float transpiredTime;
		bool waitForEnd;
		bool done;
		unsigned int toIndex;
	};

public:
	AnimationComponent(AnimationStack* animationStack);
	~AnimationComponent();
	
	void setAnimation(const unsigned int index, const bool allowTransitionWait = true);
	void setAnimation(const std::string& name);
	AnimationStack* getAnimationStack();
#ifdef DEVELOPMENT
	const unsigned int getByteSize() const override {
		/* TODO: Fix component size */
		return sizeof(*this);
	}
	void imguiRender(Entity** selected);
#endif

public:
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
	bool is_camFollowingHead;
	bool updateDT;

	Entity* rightHandEntity;
	Entity* leftHandEntity;

	glm::mat4 rightHandPosition;	// -0.55, 1.03, 0.11
	glm::mat4 leftHandPosition;		// 0.57, 1.03, 0.11
	glm::mat4 headPositionMatrix;

	glm::vec3 headPositionLocalCurrent;
	glm::vec3 headPositionLocalDefault;

	Mesh::Data data;
	glm::mat4* transforms;

	std::unique_ptr<VertexBuffer> tposeVBuffer;

	Transition currentTransition;

	// Used for upper body rotation
	float pitch = 0.f;

	// For some reason needed
	bool throwAnimationDone = false;

private:
	AnimationStack* m_stack;
};