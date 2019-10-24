#pragma once
#include "Component.h"

#include "Sail/netcode/NetcodeTypes.h"

class Entity;

// TODO: Remove as many functions as possible

// This component will eventually contain the health etc of the candles
class CandleComponent : public Component<CandleComponent> {
public:
	CandleComponent();
	virtual ~CandleComponent();

	void hitWithWater(float damage, Netcode::PlayerID shooterID);
	void resetHitByWater();
	bool wasHitByWater() const;
	bool getIsAlive() const;
	bool* getPtrToIsLit();
	void setIsAlive(bool alive);
	bool getDoActivate() const;
	void activate();
	void resetDoActivate();
	void addToDownTime(float time);
	void resetDownTime();
	bool isCarried() const;
	void setWasCarriedLastUpdate(const bool wasCarried);
	bool getWasCarriedLastUpdate() const;
	void setCarried(bool b);
	float getDownTime() const;
	bool getIsLit() const;
	void setIsLit(const bool isLit);
	int getNumRespawns() const;
	void incrementRespawns();
	void setOwner(Netcode::PlayerID playerEntityID);
	Netcode::PlayerID getOwner() const;
	int getDamageTakenLastHit() const;
	float getInvincibleTimer() const;
	void decrementInvincibleTimer(const float dt);
	void setInvincibleTimer(const float time);
	float getHealth() const;
	void setHealth(const float health);
	void decrementHealth(const float health);
	void setWasHitByNetID(Netcode::PlayerID netIdOfPlayerWhoHitThisCandle);
	unsigned char getWasHitByNetID();

	bool hitByLocalPlayer = false;

	
	Entity* m_ptrToOwner = nullptr;

private:
	bool m_wasHitByWater = false;
	float m_damageTakenLastHit = 0;
	bool m_isAlive = true;
	bool m_activate = false;
	bool m_carried = true;
	bool m_wasCarriedLastUpdate = true;
	Netcode::PlayerID wasHitByPlayerID = 0;

	float m_invincibleTimer = -2.f;
	// TODO: Replace using game settings when that is implemented
	float m_health = 20.f;

	/* Should probably be removed later */
	float m_downTime = 0.f;

	bool m_isLit = true;
	int m_respawns = 0;

	Netcode::PlayerID m_playerEntityID;
};