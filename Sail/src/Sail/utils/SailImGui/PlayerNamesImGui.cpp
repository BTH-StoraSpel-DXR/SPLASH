#include "pch.h"
#include "PlayerNamesImGui.h"
#include "Sail/entities/components/TransformComponent.h"
#include "Sail/entities/components/BoundingBoxComponent.h"
#include "Sail/entities/components/NetworkReceiverComponent.h"
#include "Network/NWrapperSingleton.h"

#include "Sail/Application.h"

const float MIN_TIME = 1.0f;

PlayerNamesImGui::PlayerNamesImGui(bool showWindow) {
}

PlayerNamesImGui::~PlayerNamesImGui() {
}

void PlayerNamesImGui::renderWindow() {
	if (m_drawPlayers.empty()) {
		return;
	}
	
	int counter = 0;
	float dist = 0;
	float minDist = 3;
	for (int i = 0; i < m_drawPlayers.size(); i++) {
		Entity* player = m_drawPlayers[i].playerEntity;
		const float& timeLeft = m_drawPlayers[i].timeLeft;

		if (player->isAboutToBeDestroyed()) {
			m_drawPlayers.erase(m_drawPlayers.begin() + i);
			i--;
			continue;
		}

		counter++;
		TransformComponent* trans = player->getComponent<TransformComponent>();
		BoundingBoxComponent* col = player->getComponent<BoundingBoxComponent>();

		glm::vec2 screenPos;
		if (trans) {
			glm::vec4 wpos = trans->getRenderMatrix()[3];

			if (col) {
				wpos.y += col->getBoundingBox()->getHalfSize().y * 2 + 0.2f;
			}

			screenPos = worldToScreen(wpos);
		}

		if (ImGui::Begin(std::string("PlayerName##" + std::to_string(counter)).c_str(), NULL, flags)) {
			NetworkReceiverComponent* nrc = player->getComponent<NetworkReceiverComponent>();
			ReplayReceiverComponent*  rrc = player->getComponent<ReplayReceiverComponent>();

			if (nrc || rrc) {
				Netcode::PlayerID owner = Netcode::getComponentOwner(nrc ? nrc->m_id : rrc->m_id);
				Player* p = NWrapperSingleton::getInstance().getPlayer(owner);
				glm::vec3 color = Application::getInstance()->getSettings().getColor(Application::getInstance()->getSettings().teamColorIndex(p->team));
				float alpha = 1;
				float scale = 1.5;

				float ww = Application::getInstance()->getWindow()->getWindowWidth();
				float wh = Application::getInstance()->getWindow()->getWindowHeight();
				bool outsideScreen = screenPos.x < 0 || screenPos.x > ww || screenPos.y < 0 || screenPos.y > wh;
				bool wrongDirection = false;

				screenPos.x = std::clamp(screenPos.x, 20.0f, ww - 10.0f);
				screenPos.y = std::clamp(screenPos.y, 20.0f, wh - 10.0f);

				if (m_localPlayer) {
					TransformComponent* localTrans = m_localPlayer->getComponent<TransformComponent>();
					if (localTrans) {
						glm::vec3 delta = trans->getRenderMatrix()[3] - localTrans->getRenderMatrix()[3];
						wrongDirection = glm::dot(m_camera->getDirection(), delta) < -0.8f;
						dist = glm::length(delta);
						if (m_maxDist > 0) {
							//Fade with distance
							if(dist < minDist){
								alpha = 1;
							} else {
								alpha = 1 - (dist - minDist) / m_maxDist;
								scale = 1.5 - (dist - minDist) / m_maxDist;
							}

							//Fade with time
							alpha *= std::clamp(timeLeft * 2, 0.0f, 1.0f);
						} else {
							float f = std::max((dist - minDist) / 10.0f, 0.0f);
							scale = std::max(1.5f - f, 1.0f);
							screenPos.y -= std::min(20.0f * f, 10.0f);
						}	
					}
				}

				if (!wrongDirection) {
					ImGui::PushStyleColor(0, ImVec4(color.x, color.y, color.z, alpha));
					if (outsideScreen) {
						ImGui::SetWindowFontScale(2);
						ImGui::Text("*");
						ImGui::SetWindowFontScale(1);
					} else {
						ImGui::SetWindowFontScale(scale);
						ImGui::Text(p->name.c_str());
						ImGui::SetWindowFontScale(1);
					}
					ImGui::PopStyleColor();
				}


			} else {
				ImGui::Text(" ");
			}

			ImVec2 s = ImGui::GetWindowSize();
			ImGui::SetWindowPos(ImVec2(screenPos.x - s.x * 0.5, screenPos.y - s.y * 0.5));
		}
		ImGui::End();

	}


}

void PlayerNamesImGui::update(float dt) {
	for (size_t i = 0; i < m_drawPlayers.size(); i++) {
		if (m_drawPlayers[i].playerEntity->isAboutToBeDestroyed() || m_drawPlayers[i].timeLeft < 0) {
			m_drawPlayers.erase(m_drawPlayers.begin() + i);
			i--;
		} else {
			m_drawPlayers[i].timeLeft -= dt;
		}
	}
}

void PlayerNamesImGui::setLocalPlayer(Entity* player) {
	m_localPlayer = player;
}

void PlayerNamesImGui::addPlayerToDraw(Entity* player) {
	auto transC = player->getComponent<TransformComponent>();
	if (player && !player->isAboutToBeDestroyed() && transC && transC->getParent() == nullptr) {
		bool notFound = true;
		for (auto& e : m_drawPlayers) {
			if (e.playerEntity->getID() == player->getID()) {
				e.timeLeft = MIN_TIME;
				notFound = false;
				break;
			}
		}

		if (notFound) {
			m_drawPlayers.push_back({player, MIN_TIME});
		}
	}
}

void PlayerNamesImGui::clearPlayersToDraw() {
	m_drawPlayers.clear();
}

void PlayerNamesImGui::setCamera(Camera* camera) {
	m_camera = camera;
}

void PlayerNamesImGui::setMaxDistance(float dist) {
	m_maxDist = dist;
}

glm::vec2 PlayerNamesImGui::worldToScreen(const glm::vec3& coord) {
	glm::vec4 screenPos = glm::vec4(coord, 1) * glm::transpose(m_camera->getViewProjection());
	screenPos /= screenPos.w;
	screenPos.y *= -1;
	screenPos += glm::vec4(1, 1, 0, 0);
	screenPos *= 0.5;
	screenPos *= glm::vec4(Application::getInstance()->getWindow()->getWindowWidth(), Application::getInstance()->getWindow()->getWindowHeight(), 0, 0);
	return screenPos;
}
