#pragma once
#include "..//BaseComponentSystem.h"
#include "Sail/netcode/NetworkedStructs.h"
#include "Sail/events/EventReceiver.h"

class SanitySystem final : public BaseComponentSystem, public EventReceiver {
public:
	SanitySystem();
	~SanitySystem();

	void update(float dt) override;	
	void updateSanityNetworked(Netcode::ComponentID id, float sanity);

	float m_maxSanity = 100;
	float m_minSanity = 0;
private:
	bool onEvent(const Event& event) override;
};