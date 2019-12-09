#pragma once
#include "../BaseComponentSystem.h"
#include "Sail/events/EventReceiver.h"

class PlayerSystem final : public BaseComponentSystem, public EventReceiver {
public:
	PlayerSystem();
	~PlayerSystem();
	void update(float dt) override;

	std::vector<Entity*>* getPlayers();

#ifdef DEVELOPMENT

	void imguiPrint(Entity** selectedEntity = nullptr) override;
#endif

private:
	bool onEvent(const Event& event) override;
};