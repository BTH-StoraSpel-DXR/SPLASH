#pragma once

#include "Component.h"

class SpectatorComponent : public Component<SpectatorComponent> {
public:
	SpectatorComponent() {}
	~SpectatorComponent() {}

	bool lookAt = false;
	glm::vec3 lookAtVector = { 0.f, 0.f, 0.f };

#ifdef DEVELOPMENT
	void imguiRender(Entity** selected) {
		if (ImGui::Button("Switch look at")) {
			lookAt = !lookAt;
		}
		ImGui::DragFloat3("Look at", &lookAtVector.x, 0.1f);
	}

	const unsigned int getByteSize() const override {
		return sizeof(*this);
	}
#endif
};