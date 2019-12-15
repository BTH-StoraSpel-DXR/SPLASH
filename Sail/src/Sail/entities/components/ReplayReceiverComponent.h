
#pragma once
#include "Component.h"
#include "Sail/netcode/NetworkedStructs.h"

// Used to identify entities that are included in the killcam
class ReplayReceiverComponent : public Component<ReplayReceiverComponent> {
public:
	ReplayReceiverComponent(Netcode::ComponentID id, Netcode::EntityType type) : m_id(id), m_entityType(type) {}
	virtual ~ReplayReceiverComponent() {}

	Netcode::ComponentID m_id;
	Netcode::EntityType m_entityType;
	bool m_wasAlive = true;

#ifdef DEVELOPMENT
	const unsigned int getByteSize() const override {
		return sizeof(*this);
	}
	void imguiRender(Entity** selected) {
		ImGui::Text(std::string("m_id: " + std::to_string(m_id)).c_str());
		ImGui::Text(std::string("m_entityType: " + (m_entityType == Netcode::EntityType::PLAYER_ENTITY) ? "PLAYER_ENTITY" : "OTHER_ENTITY").c_str());
	}
#endif
};