#pragma once
#include "Component.h"

class WaterCleaningComponent : public Component<WaterCleaningComponent> {
public:
	WaterCleaningComponent() {}
	~WaterCleaningComponent() {}

	bool isCleaning = true;
	float amountCleaned = 0.f;

#ifdef DEVELOPMENT
	const unsigned int getByteSize() const override {
		return sizeof(*this);
	}
	void imguiRender(Entity** selected) {
		ImGui::Text(("amountCleaned " + std::to_string(amountCleaned) + "L").c_str());
		ImGui::Columns(2);
		ImGui::Checkbox("##isCleaning", &isCleaning); ImGui::NextColumn();
		ImGui::Text("isCleaning");
		ImGui::Columns(1);
	}
#endif

};