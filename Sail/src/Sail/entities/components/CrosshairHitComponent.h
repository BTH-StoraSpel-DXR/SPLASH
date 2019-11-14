#pragma once

#include "Component.h"

class CrosshairHitComponent : public Component<CrosshairHitComponent> {
public:
	CrosshairHitComponent();
	CrosshairHitComponent(float durationOfAlteredCrosshair);
	~CrosshairHitComponent();

#ifdef DEVELOPMENT
	void imguiRender(Entity** selected) {
		std::string tempString = "";
		// Hit someone
		tempString = "hitSomeone : " + std::to_string(recentlyHitSomeone);
		ImGui::Checkbox(tempString.c_str(), &recentlyHitSomeone);
	}
#endif

	bool recentlyHitSomeone = false;
	float passedTimeSinceAlteration = 0.0f;
	float durationOfAlteredCrosshair = 0.5f;	// 0.5 by default unless otherwise set by constructor
};