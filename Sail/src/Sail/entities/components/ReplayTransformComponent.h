#pragma once

#include "Component.h"
#include "Sail/graphics/geometry/Transform.h"

class ReplayTransformComponent : public Component<ReplayTransformComponent>, public Transform {
public:
	ReplayTransformComponent(
		const glm::vec3& translation = { 0.0f, 0.0f, 0.0f },
		const glm::vec3& rotation    = { 0.0f, 0.0f, 0.0f },
		const glm::vec3& scale       = { 1.0f, 1.0f, 1.0f },
		ReplayTransformComponent* parent = nullptr)
		: Transform(translation, rotation, scale, parent) {
	}

	virtual ~ReplayTransformComponent() {}

#ifdef DEVELOPMENT
	void imguiRender(Entity** selected) {
		ImGui::Text("Position"); ImGui::SameLine();
		glm::vec3 pos = getTranslation();
		if (ImGui::DragFloat3("##allPos", &pos.x, 0.1f)) {
			setTranslation(pos);
		}
		ImGui::Text("Rotation"); ImGui::SameLine();
		glm::vec3 rot = getRotations();
		if (ImGui::DragFloat3("##allRot", &rot.x, 0.1f)) {
			setRotations(rot);
		}
		ImGui::Text("Scale   "); ImGui::SameLine();
		glm::vec3 scale = getScale();
		if (ImGui::DragFloat3("##allScale", &scale.x, 0.1f)) {
			setScale(scale);
		}
	}
#endif
};