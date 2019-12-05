#include "pch.h"

#include "SprinklerSystem.h"


#include "Sail/entities/components/SpotlightComponent.h"
#include "Sail/entities/ECS.h"
#include "Sail/utils/Utils.h"
#include "Sail.h"
#include "Sail/entities/systems/Gameplay/LevelSystem/LevelSystem.h"
#include "Network/NWrapperSingleton.h"

SprinklerSystem::SprinklerSystem() : BaseComponentSystem() {
	registerComponent<CandleComponent>(true, true, true);
	registerComponent<TransformComponent>(true, true, false);
	registerComponent<RenderInActiveGameComponent>(true, false, false);

	m_map = ECS::Instance()->getSystem<LevelSystem>();
	m_settings = &Application::getInstance()->getSettings();
	m_endGameStartLimit = m_settings->gameSettingsDynamic["map"]["sprinklerTime"].value;
	m_endGameTimeIncrement = m_settings->gameSettingsDynamic["map"]["sprinklerIncrement"].value;
}

SprinklerSystem::~SprinklerSystem() {

}

void SprinklerSystem::stop() {
	m_addNewSprinklers = false;
	m_enableSprinklers = false;
	m_endGameTimer = 0.f;
	m_endGameMapIncrement = 0;
	m_xMinIncrement = 0;
	m_xMaxIncrement = 0;
	m_yMinIncrement = 0;
	m_yMaxIncrement = 0;
	m_mapSide = 0;
	m_activeRooms.clear();
	m_activeSprinklers.clear();
	m_roomsToBeActivated.clear();
	m_sprinklers.clear();
}

void SprinklerSystem::update(float dt) {
	if (m_settings->gameSettingsStatic["map"]["sprinkler"].selected == 0) {
		if (m_enableSprinklers) {
			m_endGameTimer += dt;

			// Randomize a water spot with a ray for each active sprinkler
			for (int i = 0; i < m_sprinklers.size(); i++) {
				if (m_sprinklers[i].active) {
					Octree::RayIntersectionInfo tempInfo;

					float sprinklerXspread = (m_sprinklers[i].size.x * 0.5f) * m_map->tileSize;
					float sprinklerZspread = (m_sprinklers[i].size.y * 0.5f) * m_map->tileSize;

					glm::vec3 waterDir = glm::vec3(((2.f * Utils::rnd()) - 1.0f) * sprinklerXspread, -m_sprinklers[i].pos.y, ((2.f * Utils::rnd()) - 1.0f) * sprinklerZspread) + m_sprinklers[i].pos;
					waterDir = glm::normalize(waterDir - m_sprinklers[i].pos);
					m_octree->getRayIntersection(m_sprinklers[i].pos, waterDir, &tempInfo);
					glm::vec3 hitPos = m_sprinklers[i].pos + waterDir * tempInfo.closestHit;
					Application::getInstance()->getRenderWrapper()->getCurrentRenderer()->submitWaterPoint(hitPos);
				}
			}

			for (auto& e : entities) {

				// Locate player candle
				CandleComponent* candle = e->getComponent<CandleComponent>();
				TransformComponent* transform = e->getComponent<TransformComponent>();

				float candlePosX = -1.f;
				float candlePosZ;

				if (candle->isCarried && candle->wasCarriedLastUpdate && !e->isAboutToBeDestroyed()) {
					candlePosX = transform->getParent()->getTranslation().x;
					candlePosZ = transform->getParent()->getTranslation().z;
				}
				else if(!candle->wasCarriedLastUpdate){
					candlePosX = transform->getTranslation().x;
					candlePosZ = transform->getTranslation().z;
				}

				int candleLocationRoomID = 0;
				// Check if the candle is in a room with an active sprinkler, and damage it
				if (candlePosX != -1.f) {
					candleLocationRoomID = m_map->getRoomIDFromWorldPos(candlePosX, candlePosZ);
					std::vector<int>::iterator it = std::find(m_activeSprinklers.begin(), m_activeSprinklers.end(), candleLocationRoomID);
					if (it != m_activeSprinklers.end()) {
						NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
							Netcode::MessageType::HIT_BY_SPRINKLER,
							SAIL_NEW Netcode::MessageHitBySprinkler{
								e->getParent()->getComponent<NetworkReceiverComponent>()->m_id
							}
						);
					}
				}
			}

			// Add more active rooms
			if (m_addNewSprinklers) {
				// Active rooms now start their sprinklers
				if (!m_activeRooms.empty()) {
					for (size_t i = m_activeSprinklers.size(); i < m_activeSprinklers.size() + m_roomsToBeActivated.size(); i++) {
						m_sprinklers[i].active = true;
					}
					m_activeSprinklers.insert(m_activeSprinklers.end(), m_roomsToBeActivated.begin(), m_roomsToBeActivated.end());
					
					m_roomsToBeActivated.clear();
				}


				// Rotate between sides to activate
				m_mapSide++;
				m_mapSide = m_mapSide > 4 ? 1 : m_mapSide;
				switch (m_mapSide) {
				case 1:
					for (int x = 0 + m_xMinIncrement; x < m_map->xsize - m_xMaxIncrement; x++) {
						addSprinkler(x, m_yMinIncrement, nullptr);
					}
					m_yMinIncrement++;
					break;
				case 2:
					for (int x = 0 + m_xMinIncrement; x < m_map->xsize - m_xMaxIncrement; x++) {
						addSprinkler(x, m_map->ysize - 1 - m_yMaxIncrement, nullptr);
					}
					m_yMaxIncrement++;
					break;
				case 3:
					for (int y = 0 + m_yMinIncrement; y < m_map->ysize - m_yMaxIncrement; y++) {
						addSprinkler(m_xMinIncrement, y, nullptr);
					}
					m_xMinIncrement++;
					break;
				case 4:
					for (int y = 0 + m_yMinIncrement; y < m_map->ysize - m_yMaxIncrement; y++) {
						addSprinkler(m_map->xsize - 1 - m_xMaxIncrement, y, nullptr);
					}
					m_xMaxIncrement++;
					break;
				default:
					break;
				}
				m_addNewSprinklers = false;
				m_endGameMapIncrement++;

			}
			// New time increment reached, add new rooms next update
			else if (((m_endGameTimer) / m_endGameTimeIncrement) > static_cast<float>(m_endGameMapIncrement)) {
				m_addNewSprinklers = true;
			}

		}
		else {
			// End game is reached, sprinklers starting
			if (NWrapperSingleton::getInstance().isHost()) {
				m_endGameTimer += dt;
				if (m_endGameTimer > m_endGameStartLimit) {
					m_enableSprinklers = true;
					NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
						Netcode::MessageType::ENABLE_SPRINKLERS,
						SAIL_NEW Netcode::MessageEnableSprinklers{}
					);
				}
			}
		}
	}
	

	
}

const std::vector<int>& SprinklerSystem::getActiveRooms() const
{
	return m_activeRooms;
}

void SprinklerSystem::addSprinkler(int x, int y, Entity* ownerEntity) {
	int room = m_map->getRoomID(x, y);
	if (room != 0) {
		std::vector<int>::iterator itRooms = std::find(m_activeRooms.begin(), m_activeRooms.end(), room);
		if (itRooms == m_activeRooms.end()) {
			m_activeRooms.push_back(room);
			m_roomsToBeActivated.push_back(room);
			
			// Save sprinkler worldPos, room size, and room ID
			Sprinkler& sprinkler = m_sprinklers.emplace_back();
			sprinkler.roomID = room;
			sprinkler.pos = m_map->getRoomInfo(room).center;
			sprinkler.pos.y = (m_map->tileSize * m_map->tileHeight) - 0.2f;
			sprinkler.size = m_map->getRoomInfo(room).size;
		}
	}
}

void SprinklerSystem::enableSprinklers() {
	m_enableSprinklers = true;
	m_endGameTimer = 0.f;
}

void SprinklerSystem::setOctree(Octree* octree) {
	m_octree = octree;
}

#ifdef DEVELOPMENT
unsigned int SprinklerSystem::getByteSize() const {
	unsigned int size = BaseComponentSystem::getByteSize() + sizeof(*this);
	size += m_activeRooms.size() * sizeof(int);
	size += m_activeSprinklers.size() * sizeof(int);
	size += m_roomsToBeActivated.size() * sizeof(int);
	size += m_sprinklers.size() * sizeof(Sprinkler);
	return size;
}
#endif
