#include "pch.h"
#include "ModelSubmitSystem.h"
#include "..//..//components/ModelComponent.h"
#include "..//..//components/RealTimeComponent.h"
#include "..//..//components/TransformComponent.h"
#include "..//..//components/CullingComponent.h"
#include "..//..//components/RenderInActiveGameComponent.h"
#include "..//..//components/RenderInReplayComponent.h"
#include "..//..//..//Application.h"
#include "..//..//Entity.h"

template <typename T>
ModelSubmitSystem<T>::ModelSubmitSystem() {
	registerComponent<ModelComponent>(true, true, false);
	registerComponent<TransformComponent>(true, true, false);
	registerComponent<CullingComponent>(false, true, false);
	registerComponent<T>(true, false, false);
}

template <typename T>
void ModelSubmitSystem<T>::submitAll(const float alpha, float dt) {
	if (raisingVats) {
		timePassed += dt;
	}

	Renderer* renderer = Application::getInstance()->getRenderWrapper()->getCurrentRenderer();
	for (auto& e : entities) {
		ModelComponent* model = e->getComponent<ModelComponent>();
		TransformComponent* transform = e->getComponent<TransformComponent>();
		CullingComponent* culling = e->getComponent<CullingComponent>();

		Renderer::RenderFlag flags = (model->getModel()->isAnimated()) ? Renderer::MESH_DYNAMIC : Renderer::MESH_STATIC;
		
		if ((!culling || (culling && culling->isVisible)) && model->renderToGBuffer) {
			flags |= Renderer::IS_VISIBLE_ON_SCREEN;
		}

		if (raisingVats) {
			if (e->getName() == "CloningVat") {
				raise(e);
			}
		}

		renderer->submit(
			model->getModel(), 
			e->hasComponent<RealTimeComponent>() ? transform->getMatrixWithUpdate() : transform->getRenderMatrix(alpha),
			transform->getRenderMatrixLastFrame(),
			flags,
			model->teamColorID
		);
	}
}

template<typename T>
void ModelSubmitSystem<T>::raise(Entity* e) {
	TransformComponent* transform = e->getComponent<TransformComponent>();
	if (timePassed > timeToRaise) {
		timePassed = timeToRaise;
		raisingVats = false;
	}

	float percentageRaised = timePassed / timeToRaise;
	SAIL_LOG(std::to_string(percentageRaised));
	float currentY = yFloor + (yRoof - yFloor) * percentageRaised;

	glm::vec3 translation = transform->getTranslation();
	translation.y = currentY;
	transform->setTranslation(translation);
	
	if (!raisingVats) {
		timePassed = 0.0f;
	}
}

template<typename T>
void ModelSubmitSystem<T>::raiseVats() {
	raisingVats = true;
}

template<typename T>
void ModelSubmitSystem<T>::lowerVats() {
	raisingVats = false;
	for (auto& e : entities) {
		if (e->getName() == "CloningVat") {
			TransformComponent* transform = e->getComponent<TransformComponent>();
			glm::vec3 translation = transform->getTranslation();
			translation.y = yRoof;

			translation.y = yFloor;
			transform->setTranslation(translation);
		}
	}
}

template<typename T>
void ModelSubmitSystem<T>::stopVats() {
	raisingVats = false;
}


template class ModelSubmitSystem<RenderInActiveGameComponent>;
template class ModelSubmitSystem<RenderInReplayComponent>;
