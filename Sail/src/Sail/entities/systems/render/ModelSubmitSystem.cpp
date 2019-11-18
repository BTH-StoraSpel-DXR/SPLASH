#include "pch.h"
#include "ModelSubmitSystem.h"
#include "..//..//components/ModelComponent.h"
#include "..//..//components/RealTimeComponent.h"
#include "..//..//components/TransformComponent.h"
#include "..//..//components/ReplayTransformComponent.h"
#include "..//..//components/CullingComponent.h"
#include "..//..//..//Application.h"
#include "..//..//Entity.h"

template <typename T>
ModelSubmitSystem<T>::ModelSubmitSystem() {
	registerComponent<ModelComponent>(true, true, false);
	registerComponent<T>(true, true, false);
	registerComponent<CullingComponent>(false, true, false);
}

template <typename T>
void ModelSubmitSystem<T>::submitAll(const float alpha) {
	Renderer* renderer = Application::getInstance()->getRenderWrapper()->getCurrentRenderer();
	for (auto& e : entities) {
		ModelComponent* model = e->getComponent<ModelComponent>();
		T* transform = e->getComponent<T>();
		CullingComponent* culling = e->getComponent<CullingComponent>();

		Renderer::RenderFlag flags = (model->getModel()->isAnimated()) ? Renderer::MESH_DYNAMIC : Renderer::MESH_STATIC;
		
		if ((!culling || (culling && culling->isVisible)) && model->renderToGBuffer) {
			flags |= Renderer::IS_VISIBLE_ON_SCREEN;
		}

		renderer->submit(
			model->getModel(), 
			e->hasComponent<RealTimeComponent>() ? transform->getMatrixWithUpdate() : transform->getRenderMatrix(alpha), 
			flags,
			model->teamColorID
		);
	}
}


template class ModelSubmitSystem<TransformComponent>;
template class ModelSubmitSystem<ReplayTransformComponent>;