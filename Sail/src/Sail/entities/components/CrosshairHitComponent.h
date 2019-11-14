#pragma once

#include "Component.h"



class CrosshairHitComponent : public Component<CrosshairHitComponent> {
public:
	CrosshairHitComponent() { SAIL_LOG_WARNING("Initialized CrosshairHitComponent incorrectly."); }
	CrosshairHitComponent(float durationOfAlteredCrosshair_, Model* normalModel, Model* alteredModel) : 
		durationOfAlteredCrosshair(durationOfAlteredCrosshair_),
		normalModel(normalModel),
		alteredModel(alteredModel)
	{

	}
	~CrosshairHitComponent() {

	}

#ifdef DEVELOPMENT
	void imguiRender(Entity** selected) {
		std::string tempString = "";
		// Hit someone
		tempString = "hitSomeone : " + std::to_string(recentlyHitSomeone);
		ImGui::Checkbox(tempString.c_str(), &recentlyHitSomeone);
	}
#endif
	std::string normalTexture = "";
	std::string alteredTexture = "";		 // Is supposed to be set upon constructing this component
	bool recentlyHitSomeone = false;
	float passedTimeSinceAlteration = 0.0f;
	float durationOfAlteredCrosshair = 0.0f; // Is supposed to be set upon constructing this component
};