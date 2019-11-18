#pragma once
#include "Component.h"

class SanityComponent : public Component<SanityComponent> {
public:
	SanityComponent() {}
	~SanityComponent() {}

	float sanity;

#ifdef DEVELOPMENT
	void imguiRender(Entity** selected) {
		ImGui::Columns(2);
		if (ImGui::DragFloat("##SANITY", &sanity, 0.01, 0, 1)) {

		}
		ImGui::NextColumn();
		ImGui::Text(std::string("SanityValue").c_str());
	}
#endif
};