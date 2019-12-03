#include "pch.h"
#include "CameraRecordSystem.h"
#include "..//..//components/NoEntityComponent.h"
#include "imgui.h"
#include "..//..//..//Application.h"

CameraRecordSystem::CameraRecordSystem() : m_replaycam(90.f, 1280.f / 720.f, 0.1f, 5000.f) {
	registerComponent<NoEntityComponent>(true, false, false);
}

void CameraRecordSystem::setCamera(Camera* camera) {
	m_originalCam = camera;
}

void CameraRecordSystem::submitCamera(Camera* camera) {
	m_checkpoints.emplace_back(Checkpoint{camera->getPosition(), camera->getDirection(), m_curTime});
}

void CameraRecordSystem::update(float dt) {

	if (!m_checkpoints.empty()) {
		m_curTime += dt;
	}

	if (m_replay) {
		if (m_currentCheckpoint < (int)m_checkpoints.size() - 1) {
			float alpha = (m_checkpoints[m_currentCheckpoint + 1].timeStamp - m_curTime) / (m_checkpoints[m_currentCheckpoint + 1].timeStamp - m_checkpoints[m_currentCheckpoint].timeStamp);

			glm::vec3 pos = (m_checkpoints[m_currentCheckpoint + 1].pos - m_checkpoints[m_currentCheckpoint].pos) * alpha;
			glm::vec3 dir = (m_checkpoints[m_currentCheckpoint + 1].direction - m_checkpoints[m_currentCheckpoint].direction) * alpha;

			m_replaycam.setPosition(pos);
			m_replaycam.setDirection(dir);

			if (alpha >= 1) {
				m_currentCheckpoint++;
			}
		} else {
			m_currentCheckpoint++;
			stopReplay();
		}
	}
}

void CameraRecordSystem::renderImGUI() {
	ImGui::Begin("Camera Record System");

	float nextCheckpoint = (m_replay && m_currentCheckpoint < ((int)m_checkpoints.size()) - 2 ? (m_checkpoints[m_currentCheckpoint + 1].timeStamp - m_curTime) : 0);
	ImGui::Text((std::string("Current time: ") + std::to_string(m_curTime)).c_str());
	ImGui::Text((std::string("Replay Lenght: ") + (m_checkpoints.empty() ? "N/A" : std::to_string(m_checkpoints.back().timeStamp - m_checkpoints.front().timeStamp))).c_str());
	ImGui::Text((std::string("Next Checkpoint Reached In: ") + std::to_string(nextCheckpoint)).c_str());
	ImGui::Text((std::string("Checkpoints: ") + std::to_string(m_checkpoints.size())).c_str());
	ImGui::Text((std::string("Replay Active: ") + ((m_replay) ? "True" : "False")).c_str());
	ImGui::Text((std::string("Current Checkpoint: ") + std::to_string(m_currentCheckpoint) + " / " + std::to_string(m_checkpoints.size())).c_str());
	if(ImGui::Button("Add checkpoint")) {
		submitCamera(m_originalCam);
	}

	if (m_checkpoints.empty()) {
		ImGui::PushStyleVar(ImGuiSelectableFlags_Disabled, true);
	}
	if(ImGui::Button((m_replay) ? "Stop Replay" : "Start Replay")) {
		if (m_replay) {
			stopReplay();
		} else {
			startReplay();
		}
	}
	if (m_checkpoints.empty()) {
		ImGui::PopStyleVar();
	}

	ImGui::End();
}

void CameraRecordSystem::startReplay() {
	if (m_checkpoints.empty()) {
		return;
	}

	m_curTime2 = m_curTime;
	m_curTime = 0;
	m_currentCheckpoint = 0;
	m_replay = true;

	m_replaycam.setPosition(m_checkpoints[0].pos);
	m_replaycam.setDirection(m_checkpoints[0].direction);
}

void CameraRecordSystem::stopReplay() {
	m_replay = false;
	m_curTime = m_curTime2;
}

bool CameraRecordSystem::isInReplay() {
	return m_replay;
}

void CameraRecordSystem::stop() {
	m_checkpoints.clear();
	m_replay = 0;
	m_curTime = m_curTime2 = m_currentCheckpoint = 0;

	m_originalCam = nullptr;
}

Camera& CameraRecordSystem::getCamera() {
	return m_replaycam;
}
