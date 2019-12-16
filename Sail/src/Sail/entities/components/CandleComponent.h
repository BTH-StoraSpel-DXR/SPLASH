#pragma once
#include "Component.h"

#include "Sail/netcode/NetcodeTypes.h"
#include "Sail/netcode/NetworkedStructs.h"


class Entity;

// TODO: Replace with game settings
constexpr float MAX_HEALTH = 6.f;


// TODO: Remove as many functions as possible
// This component will eventually contain the health etc of the candles
class CandleComponent : public Component<CandleComponent> {
public:
	enum class DamageSource{
		NO_CLUE = 0,
		PLAYER,
		SPRINKLER,
		INSANE,
	};

	CandleComponent() {}
	virtual ~CandleComponent() {}



	void kill(DamageSource source, Netcode::ComponentID killerID) {
		health = 0;
		wasHitThisTick = true;
		lastDamageSource = source;
		wasHitByEntity = killerID;
		wasHitByPlayerID = Netcode::getComponentOwner(killerID);
	}

	// This function is only called by the host
	int hitWithWater(float damage, DamageSource source, Netcode::ComponentID hitByEntity) {
		if (health > 0.0f && invincibleTimer <= 0.0f) {
			invincibleTimer = 0.1f; // TODO: Replace 0.4f with game settings
			health -= damage;
			wasHitByEntity = hitByEntity;
			wasHitByPlayerID = Netcode::getComponentOwner(hitByEntity);
			wasHitThisTick = true;
			return damage;
		}
		return 0;
	}
#ifdef DEVELOPMENT
	const unsigned int getByteSize() const override {
		return sizeof(*this);
	}
	void imguiRender(Entity** e) override;
#endif

public:
	Entity* ptrToOwner = nullptr;

	bool hitByLocalPlayer   = false;
	bool wasHitByMeThisTick = false;
	bool wasHitByWater = false;
	bool isAlive       = true;
	bool isCarried     = true;
	bool wasCarriedLastUpdate = true;
	bool isLit                = true;
	bool userReignition       = false;

	/* Should probably be removed later */
	float downTime = 0.f;
	float invincibleTimer = 0.f;
	// TODO: Replace using game settings when that is implemented
	float health = MAX_HEALTH;

	float candleToggleTimer = 0.0f;

	int respawns = 0;

	bool wasJustExtinguished = false;
	bool wasHitThisTick      = false;
	Netcode::PlayerID playerEntityID;
	Netcode::PlayerID wasHitByPlayerID = 0;
	Netcode::ComponentID wasHitByEntity = 0;
	DamageSource lastDamageSource;
};