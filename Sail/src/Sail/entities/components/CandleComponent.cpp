#include "pch.h"
#include "CandleComponent.h"

CandleComponent::CandleComponent() {

}

CandleComponent::~CandleComponent() {

}

void CandleComponent::hitWithWater(float damage, unsigned char shooterID) {
	if (getInvincibleTimer() <= 0.f) {
		if (m_health > 0.0f) {
			setInvincibleTimer(0.4f);
			decrementHealth(damage);
			m_damageTakenLastHit = damage;
			m_wasHitByWater = true;

			if (m_health <= 0.0f) {
				setWasHitByNetID(shooterID);
			}
		}
	}
}

void CandleComponent::resetHitByWater() {
	m_wasHitByWater = false;
}

bool CandleComponent::wasHitByWater() const {
	return m_wasHitByWater;
}

bool CandleComponent::getIsAlive() const {
	return m_isAlive;
}

bool* CandleComponent::getPtrToIsLit() {
	return &m_isLit;
}

void CandleComponent::setIsAlive(bool alive) {
	if ( !m_isAlive && alive ) {
		m_activate = true;
		m_isAlive = alive;
	}
	m_isAlive = alive;
}

bool CandleComponent::getDoActivate() const {
	return m_activate;
}

void CandleComponent::resetDoActivate() {
	m_activate = false;
}

void CandleComponent::activate() {
	if ( !m_activate && m_isAlive && !m_isLit ) {
		m_activate = true;
	}
}

void CandleComponent::addToDownTime(float time) {
	m_downTime += time;
}

void CandleComponent::resetDownTime() {
	m_downTime = 0.f;
}

float CandleComponent::getDownTime() const {
	return m_downTime;
}

bool CandleComponent::getIsLit() const {
	return m_isLit;
}

void CandleComponent::setIsLit(const bool isLit) {
	m_isLit = isLit;
}

int CandleComponent::getNumRespawns() const {
	return m_respawns;
}

void CandleComponent::incrementRespawns() {
	m_respawns++;
}

void CandleComponent::setOwner(int playerEntityID) {
	m_playerEntityID = playerEntityID;
}

int CandleComponent::getOwner() const {
	return m_playerEntityID;
}

int CandleComponent::getDamageTakenLastHit() const {
	return m_damageTakenLastHit; // TODO: Check this - implicit cast from float to int
}

float CandleComponent::getInvincibleTimer() const {
	return m_invincibleTimer;
}

void CandleComponent::decrementInvincibleTimer(const float dt) {
	m_invincibleTimer -= dt;
}

void CandleComponent::setInvincibleTimer(const float time) {
	m_invincibleTimer = time;
}

float CandleComponent::getHealth() const {
	return m_health;
}

void CandleComponent::setHealth(const float health) {
	m_health = health;
}

void CandleComponent::decrementHealth(const float health) {
	m_health -= health;
}

void CandleComponent::setWasHitByNetID(unsigned char netIdOfPlayerWhoHitThisCandle) {
	wasHitByPlayerID = netIdOfPlayerWhoHitThisCandle;
}

unsigned char CandleComponent::getWasHitByNetID() {
	return wasHitByPlayerID;
}

bool CandleComponent::isCarried() const {
	return m_carried;
}

void CandleComponent::setWasCarriedLastUpdate(const bool wasCarried) {
	m_wasCarriedLastUpdate = wasCarried;
}

bool CandleComponent::getWasCarriedLastUpdate() const {
	return m_wasCarriedLastUpdate;
}

void CandleComponent::setCarried(bool b) {
	m_carried = b;
}