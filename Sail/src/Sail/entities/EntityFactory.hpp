#pragma once
#include "Sail/entities/ECS.h"

#include "Sail/netcode/NetcodeTypes.h"

class Model;
class NodeSystem;
class Shader;

namespace EntityFactory {
	Entity::SPtr CreateCandle(const std::string& name, const glm::vec3& lightPos, size_t lightIndex);
	
	
	Entity::SPtr CreateMyPlayer(Netcode::PlayerID playerID, size_t lightIndex, glm::vec3 spawnLocation);
	void CreateOtherPlayer(Entity::SPtr otherPlayer, Netcode::ComponentID netComponentID, size_t lightIndex, glm::vec3 spawnLocation);
	
	void CreateGenericPlayer(Entity::SPtr playerEntity, size_t lightIndex, glm::vec3 spawnLocation);
	
	Entity::SPtr CreateBot(Model* boundingBoxModel, Model* characterModel, const glm::vec3& pos, Model* lightModel, size_t lightIndex, NodeSystem* ns);
	Entity::SPtr CreateStaticMapObject(const std::string& name, Model * model, Model* boundingBoxModel, const glm::vec3& pos = glm::vec3(0,0,0), const glm::vec3& rot = glm::vec3(0,0,0), const glm::vec3& scale = glm::vec3(1,1,1));
	Entity::SPtr CreateProjectile(
		const glm::vec3& pos, const glm::vec3& velosity,
		bool hasLocalOwner = false, Netcode::ComponentID ownersNetId = 0,
		float lifetime = 4, float randomSpreed = 0.15f
	);
}