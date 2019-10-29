#include "pch.h"
#include "AnimationSystem.h"
#include "Sail/entities/Entity.h"
#include "Sail/entities/components/AnimationComponent.h"
#include "Sail/entities/components/ModelComponent.h"
#include "Sail/entities/components/TransformComponent.h"
#include "Sail/graphics/geometry/Model.h"
#include "sail/api/VertexBuffer.h"
#include "API/DX12/DX12API.h"
#include "API/DX12/DX12VertexBuffer.h"
#include "API/DX12/resources/DescriptorHeap.h"
#include "Sail/Application.h"
#include "Sail/graphics/shader/compute/AnimationUpdateComputeShader.h"
#include "API/DX12/DX12Utils.h"
#include <glm/gtx/compatibility.hpp>



//TODO: REMOVE ------------

#include "Sail/entities/EntityFactory.hpp"
#include "Sail/graphics/shader/material/WireframeShader.h"
#include "Sail/graphics/shader/dxr/GBufferOutShader.h"
// --------------------------

AnimationSystem::AnimationSystem() 
	: BaseComponentSystem()
	, m_interpolate(true) 
{
	// TODO: System owner should check if this is correct
	registerComponent<AnimationComponent>(true, true, true);
	registerComponent<ModelComponent>(true, true, true);
	registerComponent<TransformComponent>(false, true, true);

	m_updateShader = &Application::getInstance()->getResourceManager().getShaderSet<AnimationUpdateComputeShader>();
	m_dispatcher = std::unique_ptr<ComputeShaderDispatcher>(ComputeShaderDispatcher::Create());
	m_inputLayout = std::unique_ptr<InputLayout>(InputLayout::Create());
	m_inputLayout->pushVec3(InputLayout::POSITION, "POSITION", 0);
	m_inputLayout->pushVec2(InputLayout::TEXCOORD, "TEXCOORD", 0);
	m_inputLayout->pushVec3(InputLayout::NORMAL, "NORMAL", 0);                
	m_inputLayout->pushVec3(InputLayout::TANGENT, "TANGENT", 0);
	m_inputLayout->pushVec3(InputLayout::BITANGENT, "BINORMAL", 0);
}

AnimationSystem::~AnimationSystem() {
}

void AnimationSystem::updateHands(const glm::vec3& lPos, const glm::vec3& rPos, const glm::vec3& lRot, const glm::vec3& rRot) {
	for (auto& e : entities) {
		AnimationComponent* ac = e->getComponent<AnimationComponent>();
		if (ac) {
			ac->leftHandPosition = glm::identity<glm::mat4>();
			ac->leftHandPosition = glm::translate(ac->leftHandPosition, lPos);
			ac->leftHandPosition = ac->leftHandPosition * glm::toMat4(glm::quat(lRot));

			ac->rightHandPosition = glm::identity<glm::mat4>();
			ac->rightHandPosition = glm::translate(ac->rightHandPosition, rPos);
			ac->rightHandPosition = ac->rightHandPosition * glm::toMat4(glm::quat(rRot));
		}
	}
}

void AnimationSystem::update(float dt) {
	updateTransforms(dt);
	updateMeshCPU();
}

void AnimationSystem::updateTransforms(const float dt) { 
	for (auto& e : entities) {
		AnimationComponent* animationC = e->getComponent<AnimationComponent>();
		addTime(animationC, dt);

		if (!animationC->currentAnimation) {
#if defined(_DEBUG)
			Logger::Warning("AnimationComponent without animation set");
#endif
			continue;
		}
		//remove empty transitions
		while (animationC->transitions.size() > 0) {
			if (animationC->transitions.front().to) {
				break;
			}
			animationC->transitions.pop();
		}
		//transition update
		if (animationC->currentTransition) {
			//transitions complete
			if (animationC->currentTransition->transpiredTime >= animationC->currentTransition->transitionTime) {
				animationC->currentAnimation = animationC->currentTransition->to;
				animationC->animationTime = animationC->currentTransition->transpiredTime;
				animationC->nextAnimation = nullptr;
				animationC->currentTransition = nullptr;
				animationC->transitions.pop();
			}
		}
		
		//set transition
		if (!animationC->currentTransition && animationC->transitions.size() > 0) {
			if (!animationC->transitions.front().waitForEnd) {
				animationC->currentTransition = &animationC->transitions.front();
				animationC->nextAnimation = animationC->currentTransition->to;
			}
			else {
				if (animationC->animationTime >= animationC->currentAnimation->getMaxAnimationTime() - animationC->transitions.front().transitionTime) {
					animationC->currentTransition = &animationC->transitions.front();
					animationC->nextAnimation = animationC->currentTransition->to;
				}
			}
		}
		const unsigned int frame00 = animationC->currentAnimation->getFrameAtTime(animationC->animationTime, Animation::BEHIND);
		const unsigned int frame01 = animationC->currentAnimation->getFrameAtTime(animationC->animationTime, Animation::INFRONT); // TODO: make getNextFrame function.
		const unsigned int frame10 = animationC->nextAnimation ? animationC->nextAnimation->getFrameAtTime(animationC->transitions.front().transpiredTime, Animation::BEHIND) : 0;
		const unsigned int frame11 = animationC->nextAnimation ? animationC->nextAnimation->getFrameAtTime(animationC->transitions.front().transpiredTime, Animation::INFRONT) : 0;

		const unsigned int transformSize = animationC->currentAnimation->getAnimationTransformSize(frame00);
		if (transformSize != animationC->transformSize) {
			Memory::SafeDeleteArr(animationC->transforms);
			animationC->transforms = SAIL_NEW glm::mat4[animationC->currentAnimation->getAnimationTransformSize(unsigned int(0))];

#if defined(_DEBUG) && defined(SAIL_VERBOSELOGGING)
			Logger::Log("AnimationSystem: Rebuilt transformarray");
#endif
		}

		const glm::mat4* transforms00 = animationC->currentAnimation->getAnimationTransform(frame00);
		const glm::mat4* transforms01 = frame01 > frame00 ? animationC->currentAnimation->getAnimationTransform(frame01) : nullptr;
		const glm::mat4* transforms10 = animationC->nextAnimation ? animationC->nextAnimation->getAnimationTransform(frame10) : nullptr;
		const glm::mat4* transforms11 = (animationC->nextAnimation && frame11 > frame10) ? animationC->nextAnimation->getAnimationTransform(frame11) : nullptr;

		//INTERPOLATE
		if (transforms00 && transforms01 && transforms10 && transforms11 && m_interpolate) {
			const float frame00Time = animationC->currentAnimation->getTimeAtFrame(frame00);
			const float frame01Time = animationC->currentAnimation->getTimeAtFrame(frame01);
			const float frame10Time = animationC->nextAnimation->getTimeAtFrame(frame10);
			const float frame11Time = animationC->nextAnimation->getTimeAtFrame(frame11);
			// weight = time - time(0) / time(1) - time(0)

			const float w0 = (animationC->animationTime - frame00Time) / (frame01Time - frame00Time);
			const float w1 = (animationC->transitions.front().transpiredTime - frame10Time) / (frame11Time - frame10Time);
			const float wt = animationC->transitions.front().transpiredTime / animationC->transitions.front().transitionTime;
			animationC->animationW = wt;
			Logger::Log(std::to_string(wt));
			glm::mat4 m0 = glm::identity<glm::mat4>();
			glm::mat4 m1 = glm::identity<glm::mat4>();

			for (unsigned int transformIndex = 0; transformIndex < transformSize; transformIndex++) {
				interpolate(m0, transforms00[transformIndex], transforms01[transformIndex], w0);
				interpolate(m1, transforms10[transformIndex], transforms11[transformIndex], w1);
				interpolate(animationC->transforms[transformIndex], m0, m1, wt);
			}

		}
		else if (transforms00 && transforms10) {
			const float wt = animationC->transitions.front().transpiredTime / animationC->transitions.front().transitionTime;
			for (unsigned int transformIndex = 0; transformIndex < transformSize; transformIndex++) {
				interpolate(animationC->transforms[transformIndex], transforms00[transformIndex], transforms10[transformIndex], wt);
				animationC->transforms[transformIndex] = transforms00[transformIndex];
			}
		}
		else if (transforms00 && transforms01 && m_interpolate) {
			const float frame0Time = animationC->currentAnimation->getTimeAtFrame(frame00);
			const float frame1Time = animationC->currentAnimation->getTimeAtFrame(frame01);
			// weight = time - time(0) / time(1) - time(0)
			const float w = (animationC->animationTime - frame0Time) / (frame1Time - frame0Time);
			for (unsigned int transformIndex = 0; transformIndex < transformSize; transformIndex++) {
				interpolate(animationC->transforms[transformIndex], transforms00[transformIndex], transforms01[transformIndex], w);
			}

		} else if (transforms00) {
			for (unsigned int transformIndex = 0; transformIndex < transformSize; transformIndex++) {
				animationC->transforms[transformIndex] = transforms00[transformIndex];
			}
		}

		if (animationC->currentTransition) {
			animationC->currentTransition->transpiredTime += dt * animationC->animationSpeed;
		}
		animationC->hasUpdated = true;

		if (animationC->leftHandEntity) {

			glm::mat4 res = animationC->transforms[10] * animationC->leftHandPosition;
			
			glm::vec3 pos, scale;
			glm::quat rot;
			
			glm::decompose(res, scale, rot, pos, glm::vec3(), glm::vec4());
			
			animationC->leftHandEntity->getComponent<TransformComponent>()->setRotations(glm::eulerAngles(rot));
			animationC->leftHandEntity->getComponent<TransformComponent>()->setTranslation(pos);

		}

		if (animationC->rightHandEntity) {
		
			glm::mat4 res = animationC->transforms[22] * animationC->rightHandPosition;
		
			glm::vec3 pos, scale;
			glm::quat rot;
		
			glm::decompose(res, scale, rot, pos, glm::vec3(), glm::vec4());
		
			animationC->rightHandEntity->getComponent<TransformComponent>()->setRotations(glm::eulerAngles(rot));
			animationC->rightHandEntity->getComponent<TransformComponent>()->setTranslation(pos);
		
		}


	}
}

void AnimationSystem::updateMeshGPU(ID3D12GraphicsCommandList4* cmdList) {
	m_dispatcher->begin(cmdList);
	unsigned int meshIndex = 0;
	for (auto& e : entities) {
		AnimationComponent* animationC = e->getComponent<AnimationComponent>();
		ModelComponent* modelC = e->getComponent<ModelComponent>();

		if (!animationC->computeUpdate) {
			continue;
		}

		Mesh* mesh = modelC->getModel()->getMesh(0);
		const Mesh::Data* data = &mesh->getMeshData();

		// Initialize T-pose vertex buffer if it has not been done already
		if (!animationC->tposeVBuffer) {
			animationC->tposeVBuffer = std::unique_ptr<VertexBuffer>(DX12VertexBuffer::Create(*m_inputLayout, *data));
		}
		DX12VertexBuffer* tposeVBuffer = static_cast<DX12VertexBuffer*>(animationC->tposeVBuffer.get());
		// Make sure it has been initialized
		tposeVBuffer->init(cmdList);

		AnimationStack::VertConnection* connections = animationC->getAnimationStack()->getConnections();
		const unsigned int connectionSize = animationC->getAnimationStack()->getConnectionSize();

		auto* context = Application::getInstance()->getAPI<DX12API>();

		m_updateShader->getPipeline()->setStructBufferVar("CSTransforms", animationC->transforms, animationC->transformSize, meshIndex);
		m_updateShader->getPipeline()->setStructBufferVar("CSVertConnections", connections, connectionSize, meshIndex);


		// Get the vertexbuffer that should contain the animated mesh
		auto& vbuffer = static_cast<DX12VertexBuffer&>(mesh->getVertexBuffer());
		// Make sure vertex buffer data has been uploaded to its default buffer
		vbuffer.init(cmdList);

		if (!animationC->hasUpdated) {
			DX12Utils::SetResourceTransitionBarrier(cmdList, vbuffer.getBuffer(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
			DX12Utils::SetResourceTransitionBarrier(cmdList, vbuffer.getBuffer(-1), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_SOURCE);
			if (vbuffer.getBuffer() == vbuffer.getBuffer(-1)) {
				Logger::Error("Well this is awkward");
			}
			cmdList->CopyResource(vbuffer.getBuffer(), vbuffer.getBuffer(-1));
			//DX12Utils::SetResourceUAVBarrier(cmdList, vbuffer.getBuffer());
			DX12Utils::SetResourceTransitionBarrier(cmdList, vbuffer.getBuffer(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
			DX12Utils::SetResourceTransitionBarrier(cmdList, vbuffer.getBuffer(-1), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

			// Mark vbuffer as updated - this makes sure the BLAS gets updated
			vbuffer.setAsUpdated();
			continue;
		}

		auto& cdh = context->getComputeGPUDescriptorHeap()->getCurentCPUDescriptorHandle();
		cdh.ptr += context->getComputeGPUDescriptorHeap()->getDescriptorIncrementSize() * 2; // TODO: read offset from root params

		// Create a shader resource view for the T-pose vbuffer in the correct place in the heap
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = connectionSize;
		srvDesc.Buffer.StructureByteStride = m_inputLayout->getVertexSize();
		context->getDevice()->CreateShaderResourceView(tposeVBuffer->getBuffer(), &srvDesc, cdh);
		// Transition to read access
		DX12Utils::SetResourceTransitionBarrier(cmdList, tposeVBuffer->getBuffer(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		cdh.ptr += context->getComputeGPUDescriptorHeap()->getDescriptorIncrementSize() * 8; // TODO: read offset from root params

		// Create a unordered access view for the animated vertex buffer in the correct place in the heap
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = connectionSize;
		uavDesc.Buffer.StructureByteStride = 4 * 14; // TODO: replace with sizeof(Vertex)
		context->getDevice()->CreateUnorderedAccessView(vbuffer.getBuffer(), nullptr, &uavDesc, cdh);
		// Transition to UAV access
		DX12Utils::SetResourceTransitionBarrier(cmdList, vbuffer.getBuffer(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		AnimationUpdateComputeShader::Input input;
		input.threadGroupCountX = connectionSize * m_updateShader->getComputeSettings()->threadGroupXScale;
		m_dispatcher->dispatch(*m_updateShader, input, meshIndex, cmdList);

		// Transition back to normal vertex buffer usage
		DX12Utils::SetResourceTransitionBarrier(cmdList, vbuffer.getBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		DX12Utils::SetResourceTransitionBarrier(cmdList, tposeVBuffer->getBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		context->getComputeGPUDescriptorHeap()->getAndStepIndex(18); // Dispatch steps twice, we need to step 18 more times to align the heap for the next animated entity
																	 // TODO: read offset from root params

		vbuffer.setAsUpdated();

		animationC->hasUpdated = false;
		meshIndex++;
	}
}

void AnimationSystem::updateMeshCPU() { 
	for (auto& e : entities) {
		AnimationComponent* animationC = e->getComponent<AnimationComponent>();
		ModelComponent* modelC = e->getComponent<ModelComponent>();

		if (animationC->computeUpdate) {
			continue;
		}


		Mesh* mesh = modelC->getModel()->getMesh(0);
		const Mesh::Data* data = &mesh->getMeshData();
		if (data->numVertices > 0) {
			if (animationC->data.numVertices != data->numVertices) {
				animationC->data.deepCopy(*data);
			}
			const Mesh::vec3* pos = data->positions;
			const Mesh::vec3* norm = data->normals;
			const Mesh::vec3* tangent = data->tangents;
			const Mesh::vec3* bitangent = data->bitangents;
			const Mesh::vec2* uv = data->texCoords;

			AnimationStack::VertConnection* connections = animationC->getAnimationStack()->getConnections();
			const unsigned int connectionSize = animationC->getAnimationStack()->getConnectionSize();

			// CPU UPDATE
			if (connections && animationC->transforms) {
				glm::mat4 mat;
				glm::mat4 matInv;

				for (unsigned int connectionIndex = 0; connectionIndex < connectionSize; connectionIndex++) {
					unsigned int count = connections[connectionIndex].count;
					mat = glm::zero<glm::mat4>();
					matInv = glm::zero<glm::mat4>();

					float weightTotal = 0.0f;
					for (unsigned int countIndex = 0; countIndex < count; countIndex++) {
						mat += animationC->transforms[connections[connectionIndex].transform[countIndex]] * connections[connectionIndex].weight[countIndex];
						weightTotal += connections[connectionIndex].weight[countIndex];
					}
					matInv = glm::inverseTranspose(mat);

					animationC->data.positions[connectionIndex].vec = glm::vec3(mat * glm::vec4(pos[connectionIndex].vec, 1));
					animationC->data.normals[connectionIndex].vec = glm::vec3(matInv * glm::vec4(norm[connectionIndex].vec, 0));
					animationC->data.tangents[connectionIndex].vec = glm::vec3(matInv * glm::vec4(tangent[connectionIndex].vec, 0));
					animationC->data.bitangents[connectionIndex].vec = glm::vec3(matInv * glm::vec4(bitangent[connectionIndex].vec, 0));
					animationC->data.texCoords[connectionIndex].vec = uv[connectionIndex].vec;
				}
			}
		}
	}

}

const std::vector<Entity*>& AnimationSystem::getEntities() const {
	return entities;
}

void AnimationSystem::initDebugAnimations() {
	Application* app = Application::getInstance();
	auto* shader = &app->getResourceManager().getShaderSet<GBufferOutShader>();
	std::string name = "Doc.fbx";

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
		EntityFactory::CreateGenericPlayer(animationEntity2, i, glm::vec3(-2.0f + i * 1.2f, 0, -2));

		AnimationComponent* ac = animationEntity2->getComponent<AnimationComponent>();
		ac->setAnimation(i);
	}
}

void AnimationSystem::addTime(AnimationComponent* e, const float time) {
	e->animationTime += time * e->animationSpeed;
	if (e->animationTime >= e->currentAnimation->getMaxAnimationTime()) {
		e->animationTime -= (int(e->animationTime / e->currentAnimation->getMaxAnimationTime()) * e->currentAnimation->getMaxAnimationTime());
	}
}

void AnimationSystem::interpolate(glm::mat4& res, const glm::mat4& mat1, const glm::mat4& mat2, const float w) {
	glm::vec3 pos[2], scale[2];
	glm::quat rot[2];

	glm::decompose(mat1, scale[0], rot[0], pos[0], glm::vec3(), glm::vec4());
	glm::decompose(mat2, scale[1], rot[1], pos[1], glm::vec3(), glm::vec4());

	glm::vec3 resScale = glm::lerp(scale[0], scale[1], w);
	glm::vec3 resPos = glm::lerp(pos[0], pos[1], w);
	glm::quat resRot = glm::slerp(rot[0], rot[1], w);

	res = glm::translate(glm::identity<glm::mat4>(), resPos);
	res = glm::scale(res, resScale);
	res = res * glm::toMat4(resRot);
}

void AnimationSystem::updatePerFrame() {
	for (auto& e : entities) {
		AnimationComponent* animationC = e->getComponent<AnimationComponent>();
		if (!animationC->computeUpdate) {
			if (animationC->dataSize == 0) {
				updateMeshCPU();
			}
			ModelComponent* modelC = e->getComponent<ModelComponent>();
			Mesh* mesh = modelC->getModel()->getMesh(0);
			mesh->getVertexBuffer().update(animationC->data);
		}
	}
}

void AnimationSystem::toggleInterpolation() {
	m_interpolate = !m_interpolate;
}

const bool AnimationSystem::getInterpolation() {
	return m_interpolate;
}

void AnimationSystem::setInterpolation(const bool interpolation) {
	m_interpolate = interpolation;
}
