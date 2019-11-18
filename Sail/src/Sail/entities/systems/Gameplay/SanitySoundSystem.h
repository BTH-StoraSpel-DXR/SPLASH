#pragma once
#include "..//BaseComponentSystem.h"
#include "Sail/netcode/NetworkedStructs.h"
#include "Sail/events/EventReceiver.h"

// 0.0 <= X <= 1.0f
#define TIMEPOINT_AMBIANCE 100.0f
#define TIMEPOINT_HEART 90.f
#define TIMEPOINT_BREATHING 60.0f
#define TIMEPOINT_VIOLIN 25.0f

class SanitySoundSystem final : public BaseComponentSystem, public EventReceiver {
public:
	SanitySoundSystem();
	~SanitySoundSystem();

	void update(float dt) override;
private:
	bool onEvent(const Event& event) override;

	bool m_switch_heartBegin = false;
	bool m_switch_firstBeat = false;
	bool m_switch_secondBeat = false;
	bool m_switch_Beat = false;
	bool m_switch_ambiance = false;
	bool m_switch_breathing = false;
	bool m_switch_violin = false;

	float m_heartBeatTimer = 0.0f;
	float m_heartSecondBeatThresh = 0.45f;
	float m_heartBeatResetThresh = 1.5f;
	float m_violinTimer = 0.0f;
	float m_violinThresh = 0.60f;
};