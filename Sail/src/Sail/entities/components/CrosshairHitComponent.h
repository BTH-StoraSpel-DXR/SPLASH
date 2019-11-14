#pragma once

#include "Component.h"



class CrosshairHitComponent : public Component<CrosshairHitComponent> {
public:
	CrosshairHitComponent(float durationOfAlteredCrosshair_, Model* alteredCrosshairModel) : 
		durationOfAlteredCrosshair(durationOfAlteredCrosshair_),
		alteredModel(alteredCrosshairModel)
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

	Model* alteredModel = nullptr;			 // Is supposed to be set upon constructing this component
	bool recentlyHitSomeone = false;
	float passedTimeSinceAlteration = 0.0f;
	float durationOfAlteredCrosshair = 0.0f; // Is supposed to be set upon constructing this component
};