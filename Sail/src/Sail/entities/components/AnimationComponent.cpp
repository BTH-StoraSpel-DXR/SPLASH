#include "pch.h"
#include "AnimationComponent.h"

AnimationComponent::AnimationComponent(AnimationStack* animationStack) :
	dataSize(0),
	computeUpdate(true),
	animationTime(0),
	animationIndex(0),
	animationSpeed(1.0f),
	animationName(""),
	currentAnimation(nullptr),
	nextAnimation(nullptr),
	blending(false),
	transformSize(0),
	hasUpdated(false),
	animationW(0.0f), //TODO: REMOVE
	is_camFollowingHead(false),
	updateDT(true),
	rightHandEntity(nullptr),
	leftHandEntity(nullptr),
	headPositionLocalDefault(glm::vec3(0.02f, 1.81f, -0.01f)),
	currentTransition(nullptr),
	m_stack(animationStack)
{
	transformSize = m_stack->getAnimation(0)->getAnimationTransformSize(unsigned int(0));
	transforms = SAIL_NEW glm::mat4[transformSize];
}

AnimationComponent::~AnimationComponent() {
	Memory::SafeDeleteArr(data.indices);
	Memory::SafeDeleteArr(data.positions);
	Memory::SafeDeleteArr(data.normals);
	Memory::SafeDeleteArr(data.bitangents);
	Memory::SafeDeleteArr(data.colors);
	Memory::SafeDeleteArr(data.tangents);
	Memory::SafeDeleteArr(data.texCoords);
	Memory::SafeDeleteArr(transforms);
}

void AnimationComponent::setAnimation(const unsigned int index) {
	animationIndex = index;
	if (animationIndex > m_stack->getAnimationCount()) {
		animationIndex = 0;
	}
	currentAnimation = m_stack->getAnimation(animationIndex);
}
void AnimationComponent::setAnimation(const std::string& name) {
	animationIndex = m_stack->getAnimationIndex(name);
	currentAnimation = m_stack->getAnimation(animationIndex);
}
AnimationStack* AnimationComponent::getAnimationStack() {
	return m_stack;
}

#ifdef DEVELOPMENT
void AnimationComponent::imguiRender(Entity** selected) {
	AnimationComponent* animationC = this;
	ImGui::Text("Animation: %s", animationC->currentAnimation->getName().c_str());
	ImGui::Checkbox("Update on GPU", &animationC->computeUpdate);
	if (ImGui::SliderFloat("Animation Speed", &animationC->animationSpeed, 0.0f, 3.0f)) {
	}
	
	AnimationStack* stack = animationC->getAnimationStack();
	float w = animationC->animationW;
	ImGui::SliderFloat("weight", &w, 0.0f, 1.0f);
	ImGui::Text("AnimationStack");
	
	for (unsigned int animationTrack = 0; animationTrack < stack->getAnimationCount(); animationTrack++) {
		float time = -1;
		if (animationC->currentAnimation == stack->getAnimation(animationTrack)) {
			time = animationC->animationTime;
		}
		if (animationC->nextAnimation == stack->getAnimation(animationTrack)) {
			if (time > -1) {
				float time2 = animationC->transitions.front().transpiredTime;
				ImGui::SliderFloat(std::string("CurrentTime: " + std::to_string(animationTrack) + "T").c_str(), &time2, 0.0f, stack->getAnimation(animationTrack)->getMaxAnimationTime());
			}
			else {
				time = animationC->transitions.front().transpiredTime;
			}
		}
		if (time == -1) {
			time = 0;
		}
		ImGui::SliderFloat(std::string("CurrentTime: " + std::to_string(animationTrack)).c_str(), &time, 0.0f, stack->getAnimation(animationTrack)->getMaxAnimationTime());
		if (animationC->currentAnimation == stack->getAnimation(animationTrack)) {
			animationC->animationTime = time;
		}
	}

	static float transitionTime = 0.4f;
	static bool transitionWait = false;
	ImGui::Checkbox("transition wait", &transitionWait);
	ImGui::SameLine();
	if (ImGui::SliderFloat("Transition Time", &transitionTime, 0.0f, 1.0f)) {
	}
	for (unsigned int animationIndex = 0; animationIndex < stack->getAnimationCount(); animationIndex++) {
		if (ImGui::Button(std::string("Switch to " + stack->getAnimation(animationIndex)->getName()).c_str())) {
			animationC->transitions.emplace(stack->getAnimation(animationIndex), transitionTime, transitionWait);
		}
		ImGui::Separator();
	}

	ImGui::Text("Head pos local default"); ImGui::SameLine();
	if (ImGui::DragFloat3("##allPos", &headPositionLocalDefault.x, 0.1f)) {
		headPositionMatrix = glm::translate(glm::identity<glm::mat4>(), headPositionLocalDefault);
	}
}
#endif