#include "pch.h"

#include "RenderSettingsWindow.h"
#include "Sail/Application.h"
#include "Sail/entities/ECS.h"
#include "Sail/entities/systems/graphics/AnimationSystem.h"
#include "../Physics/Octree.h"
#include "Sail/entities/components/ModelComponent.h"
#include "Sail/entities/components/RenderInActiveGameComponent.h"
#include "Sail/graphics/camera/Camera.h"
#include "imgui.h"

RenderSettingsWindow::RenderSettingsWindow(bool showWindow) 
	: m_cam(nullptr)
	, m_octree(nullptr)
{ }

RenderSettingsWindow::~RenderSettingsWindow() {

}

void RenderSettingsWindow::activateMaterialPicking(Camera* camera, Octree* octree) {
	m_cam = camera;
	m_octree = octree;
}

void RenderSettingsWindow::renderWindow() {
	ImGui::Begin("Rendering settings");
	ImGui::Checkbox("Enable post processing",
		&(*Application::getInstance()->getRenderWrapper()).getDoPostProcessing()
	);
	bool interpolate = ECS::Instance()->getSystem<AnimationSystem<RenderInActiveGameComponent>>()->getInterpolation();
	ImGui::Checkbox("enable animation interpolation", &interpolate);
	ECS::Instance()->getSystem<AnimationSystem<RenderInActiveGameComponent>>()->setInterpolation(interpolate);

	ImGui::SliderFloat("Gamma", &Application::getInstance()->getSettings().applicationSettingsDynamic["graphics"]["gamma"].value, 0.f, 5.f);

	static Entity* pickedEntity = nullptr;
	static float metalness = 1.0f;
	static float roughness = 1.0f;
	static float ao = 1.0f;

	ImGui::Separator();
	if (m_octree && m_cam) {
		if (ImGui::Button("Pick entity")) {
			Octree::RayIntersectionInfo tempInfo;
			m_octree->getRayIntersection(m_cam->getPosition(), m_cam->getDirection(), &tempInfo);
			if (tempInfo.closestHitIndex != -1) {
				pickedEntity = tempInfo.info[tempInfo.closestHitIndex].entity;
			}
		}
	}

	if (pickedEntity) {
		ImGui::Text("Material properties for %s", pickedEntity->getName().c_str());
		if (auto* model = pickedEntity->getComponent<ModelComponent>()) {
			auto* mat = model->getModel()->getMesh(0)->getMaterial();
			const auto& pbrSettings = mat->getPBRSettings();
			metalness = pbrSettings.metalnessScale;
			roughness = pbrSettings.roughnessScale;
			ao = pbrSettings.aoScale;
			if (ImGui::SliderFloat("Metalness scale", &metalness, 0.f, 1.f)) {
				mat->setMetalnessScale(metalness);
			}
			if (ImGui::SliderFloat("Roughness scale", &roughness, 0.f, 1.f)) {
				mat->setRoughnessScale(roughness);
			}
			if (ImGui::SliderFloat("AO scale", &ao, 0.f, 1.f)) {
				mat->setAOScale(ao);
			}
		}
	}

	ImGui::End();
}
