#pragma once

#include "Sail/entities/systems/BaseComponentSystem.h"
#include "Sail/entities/systems/Gameplay/LevelSystem/LevelSystem.h"
#include "Sail/utils/Storage/SettingStorage.h"

class Octree;

struct Sprinkler {
	int roomID;
	glm::vec3 pos;
	glm::vec2 size;
	bool active = false;
};

class SprinklerSystem final : public BaseComponentSystem {
public:
	SprinklerSystem();
	~SprinklerSystem();

	void update(float dt) override;
	void stop() override;
	const std::vector<int>& getActiveRooms() const;
	void enableSprinklers();
	void setOctree(Octree* octree);

private:
	bool m_addNewSprinklers = true;
	bool m_enableSprinklers = false;
	float m_endGameTimer = 0.f;
	float m_endGameStartLimit;

	SettingStorage* m_settings;
	LevelSystem* m_map;
	Octree* m_octree;
	float m_endGameTimeIncrement;
	int m_endGameMapIncrement = 0;
	int m_xMinIncrement = 0;
	int m_xMaxIncrement = 0;
	int m_yMinIncrement = 0;
	int m_yMaxIncrement = 0;
	int m_mapSide = 0;
	std::vector<int> m_activeRooms;
	std::vector<int> m_activeSprinklers;
	std::vector<int> m_roomsToBeActivated;
	std::vector<Sprinkler> m_sprinklers;

	void addSprinkler(int x, int y);
};