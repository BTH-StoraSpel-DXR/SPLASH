#include "pch.h"
#include "AnimationInitSystem.h"
#include "Sail/graphics/shader/dxr/GBufferOutShader.h"
#include "../../ECS.h"
#include "Sail/Application.h"
#include "../../components/TransformComponent.h"
#include "../../components/ModelComponent.h"
#include "../../components/AnimationComponent.h"
#include "../../components/NoEntityComponent.h"
#include "Sail/entities/EntityFactory.hpp"
#include "Sail/graphics/shader/material/WireframeShader.h"
#include "Sail/entities/components/CandleComponent.h"
#include "Sail/entities/components/BoundingBoxComponent.h"
#include "Sail/entities/components/CollidableComponent.h"
#include "Sail/entities/components/OnlineOwnerComponent.h"
#include "Sail/entities/components/CandleComponent.h"
#include "Sail/entities/components/LightComponent.h"
#include "Sail/graphics/light/PointLight.h"

AnimationInitSystem::AnimationInitSystem() {
	registerComponent<NoEntityComponent>(true, false, false);
}

AnimationInitSystem::~AnimationInitSystem() {
}

void AnimationInitSystem::loadAnimations() {

}

void AnimationInitSystem::initAnimations() {
	Application* app = Application::getInstance();
	auto* shader = &app->getResourceManager().getShaderSet<GBufferOutShader>();
	std::string name = "DocTorch.fbx";

	auto* wireframeShader = &Application::getInstance()->getResourceManager().getShaderSet<WireframeShader>();
	Model* lightModel = &Application::getInstance()->getResourceManager().getModel("candleExported.fbx", shader);
	lightModel->getMesh(0)->getMaterial()->setAlbedoTexture("sponza/textures/candleBasicTexture.tga");
	//Wireframe bounding box model
	Model* boundingBoxModel = &Application::getInstance()->getResourceManager().getModel("boundingBox.fbx", wireframeShader);
	boundingBoxModel->getMesh(0)->getMaterial()->setColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));



	AnimationStack* stack = &app->getResourceManager().getAnimationStack(name);
	unsigned int animationCount = stack->getAnimationCount();
	for (unsigned int i = 0; i < animationCount; i++) {
		auto animationEntity2 = ECS::Instance()->createEntity("Doc" + std::to_string(i));
		animationEntity2->addComponent<TransformComponent>();
		animationEntity2->getComponent<TransformComponent>()->translate(-2.0f+i*1.2f, 0, -2);
		Model* model = &app->getResourceManager().getModelCopy(name);
		animationEntity2->addComponent<ModelComponent>(model);
		model->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Character/CharacterMRAO.tga");
		model->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Character/CharacterTex.tga");
		model->getMesh(0)->getMaterial()->setNormalTexture("pbr/Character/CharacterNM.tga");
		animationEntity2->getComponent<ModelComponent>()->getModel()->setIsAnimated(true);
		AnimationComponent* ac = animationEntity2->addComponent<AnimationComponent>(stack);
		ac->currentAnimation = stack->getAnimation(i);

		//creates light with model and pointlight
		auto light = ECS::Instance()->createEntity("ReceiverLight");
		light->addComponent<CandleComponent>();
		light->addComponent<ModelComponent>(lightModel);
		light->addComponent<TransformComponent>();
		light->addComponent<BoundingBoxComponent>(boundingBoxModel);
		//light->addComponent<CollidableComponent>();
		//PointLight pl;
		//pl.setColor(glm::vec3(0.2f, 0.2f, 0.2f));
		//pl.setPosition(glm::vec3(0.2f, 0.2f + .37f, 0.2f));
		//pl.setAttenuation(.0f, 0.1f, 0.02f);
		////pl.setIndex(m_currLightIndex++);
		//pl.setIndex(light->getID()); // TODO: unique light index needed?
		//light->addComponent<LightComponent>(pl);
		
		animationEntity2->addChildEntity(light);
		ac->leftHandEntity = light.get();

		ac->leftHandPosition = glm::identity<glm::mat4>();
		ac->leftHandPosition = glm::translate(ac->leftHandPosition, glm::vec3(0.57f, 1.03f, 0.05f));
		ac->leftHandPosition = ac->leftHandPosition * glm::toMat4(glm::quat(glm::vec3(3.14f*0.5f, -3.14f * 0.17f, 0.0f)));
		//ac->leftHandPosition = glm::identity<glm::mat4>();


		light = ECS::Instance()->createEntity("ReceiverLight2");
		//light->addComponent<CandleComponent>();
		light->addComponent<ModelComponent>(lightModel);
		light->addComponent<TransformComponent>();
		//light->addComponent<BoundingBoxComponent>(boundingBoxModel);
		//light->addComponent<CollidableComponent>();
		//PointLight pl2;
		//pl2.setColor(glm::vec3(0.2f, 0.2f, 0.2f));
		//pl2.setPosition(glm::vec3(0.2f, 0.2f + .37f, 0.2f));
		//pl2.setAttenuation(.0f, 0.1f, 0.02f);
		////pl.setIndex(m_currLightIndex++);
		//pl2.setIndex(light->getID()); // TODO: unique light index needed?
		//light->addComponent<LightComponent>(pl2);
		
		animationEntity2->addChildEntity(light);
		ac->rightHandEntity = light.get();
		
		ac->rightHandPosition = glm::identity<glm::mat4>();
		ac->rightHandPosition = glm::translate(ac->rightHandPosition, glm::vec3(-0.57f, 1.03f, 0.05));
		ac->rightHandPosition = ac->rightHandPosition * glm::toMat4(glm::quat(glm::vec3(3.14f * 0.5f, 3.14f * 0.12f, 0.0f)));
		//ac->rightHandPosition = glm::identity<glm::mat4>();

	}

}
