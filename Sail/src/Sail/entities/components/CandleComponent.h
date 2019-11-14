#pragma once
#include "Component.h"

#include "Sail/netcode/NetcodeTypes.h"

class Entity;

// TODO: Replace with game settings
constexpr float MAX_HEALTH = 20.f;


// TODO: Remove as many functions as possible
// This component will eventually contain the health etc of the candles
class CandleComponent : public Component<CandleComponent> {
public:
	CandleComponent() {}
	virtual ~CandleComponent() {}

	// This function is only called by the host
	void hitWithWater(float damage, Netcode::PlayerID shooterID) {
		if (health > 0.0f && invincibleTimer <= 0.0f) {
			invincibleTimer = 0.4f; // TODO: Replace 0.4f with game settings
			health -= damage;
			wasHitByPlayerID = shooterID;
			wasHitThisTick = true;
		}
	}
#ifdef DEVELOPMENT
	void imguiRender(Entity** e) override;
#endif

public:
	Entity* ptrToOwner = nullptr;

	bool hitByLocalPlayer = false;
	bool wasHitByMeThisTick = false;
	bool wasHitByWater = false;
	bool isAlive = true;
	bool isCarried = true;
	bool wasCarriedLastUpdate = true;
	bool isLit = true;
	bool userReignition = false;

	/* Should probably be removed later */
	float downTime = 0.f;
	float invincibleTimer = 0.f;
	// TODO: Replace using game settings when that is implemented
	float health = MAX_HEALTH;

	int respawns = 0;

	bool wasJustExtinguished = false;
	bool wasHitThisTick = false;
	Netcode::PlayerID playerEntityID;
	Netcode::PlayerID wasHitByPlayerID = 0;
};