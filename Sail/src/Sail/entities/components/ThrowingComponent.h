#pragma once
#include "Component.h"

#include <glm/vec3.hpp>


// TODO: Replace with charge animation length
constexpr auto CHARGE_AND_THROW_ANIM_LENGTH = 0.79f; // Charge animation length
constexpr auto DROP_ANIMATION_LENGTH = 0.33f;
// TODO: Replace with game settings
constexpr auto MAX_THROW_CHARGE_MULT = 20.f;

class ThrowingComponent : public Component<ThrowingComponent> {
public:
	ThrowingComponent() {}
	~ThrowingComponent() {}

public:
	bool isCharging					= false;
	bool wasChargingLastFrame		= false;
	bool isThrowing					= false;
	bool doThrow					= false;
	bool isDropping					= false;

	float chargeTime				= 0.f;
	float throwChargeMultiplier		= 12.f;
	float throwingTimer				= 0.f;
	float dropTimer					= 0.f;
	float chargeToThrowThreshold	= 0.15f;

	glm::vec3 direction				= glm::vec3(0.f, 1.f, 0.f);

public:
#ifdef DEVELOPMENT
	void imguiRender(Entity** selected) {
		if (selected) {
			ThrowingComponent* throwingC = this;
			ImGui::Checkbox("isCharging", &isCharging);
			ImGui::Checkbox("wasChargingLastFrame", &wasChargingLastFrame);
			ImGui::Checkbox("isThrowing", &isThrowing);
			ImGui::SliderFloat("chargeToThrowThreshold", &throwingC->chargeToThrowThreshold, 0.f, 1.f);
			ImGui::SliderFloat("chargeTime", &throwingC->chargeTime, 0.f, throwingC->chargeToThrowThreshold);
			ImGui::SliderFloat("throwingTimer", &throwingC->throwingTimer, 0.f, CHARGE_AND_THROW_ANIM_LENGTH);
			ImGui::SliderFloat("throwChargeMultiplier", &throwingC->throwChargeMultiplier, 0.f, MAX_THROW_CHARGE_MULT);
			ImGui::SliderFloat3("direction", &throwingC->direction.x, 0.f, 1.f);
		}
	}
#endif
};
