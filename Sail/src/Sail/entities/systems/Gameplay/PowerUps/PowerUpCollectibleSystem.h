#pragma once
#include "../../BaseComponentSystem.h"
#include "../../..//components/PowerUp/PowerUpComponent.h"
#include "Sail/events/EventReceiver.h"
#include "Sail/netcode/NetcodeTypes.h"
#include "Sail/events/Events.h"

class PowerUpCollectibleSystem final : public BaseComponentSystem, public EventReceiver {
public:
	PowerUpCollectibleSystem();
	~PowerUpCollectibleSystem();
	void init(std::vector<Entity*>* playerList);
	void setSpawnPoints(std::vector<glm::vec3>& points);
	void setRespawnTime(const float time);
	void setDuration(const float time);
	void update(float dt) override;
	void spawnPowerUps(int amount = -1);
	void spawnPowerUp(glm::vec3 pos, int powerUp, float time, float respawntime, Entity* parent = nullptr, Netcode::ComponentID compID = 0);
#ifdef DEVELOPMENT
	unsigned int getByteSize() const override;
	void imguiPrint(Entity** selectedEntity = nullptr) override;
#endif
private:
	void updateSpawns(const float dt);

	void onDestroyPowerUp(const DestroyPowerUp& e);
private:
	struct ReSpawn {
		float time;
		PowerUps powerUp;
		glm::vec3 pos;
	};
	float m_collectDistance;
	float m_respawnTime;
	float m_duration;


	std::vector<Entity*>* m_playerList;
	std::list<ReSpawn> m_respawns;
	std::list<glm::vec3> m_spawnPoints;
	std::vector<float> m_distances;

	// Inherited via EventReceiver
	virtual bool onEvent(const Event& e) override;
};