#pragma once
#include "Sail/entities/ECS.h"

#include "Sail/netcode/NetcodeTypes.h"

class Model;
class NodeSystem;
class Shader;

namespace EntityFactory {
	void CreateCandle(Entity::SPtr& candle, const glm::vec3& lightPos, size_t lightIndex);
	
	Entity::SPtr CreateWaterGun(const std::string& name);
	
	void AddCandleComponentsToPlayer(Entity::SPtr& player, const size_t& lightIndex, const Netcode::PlayerID& playerID);

	Entity::SPtr CreateMyPlayer(Netcode::PlayerID playerID, size_t lightIndex, glm::vec3 spawnLocation);

	void CreateOtherPlayer(Entity::SPtr otherPlayer, 
		Netcode::ComponentID playerCompID, 
		Netcode::ComponentID candleCompID, 
		Netcode::ComponentID gunCompID, 
		size_t lightIndex, glm::vec3 spawnLocation);
	
	void CreatePerformancePlayer(Entity::SPtr playerEnt, size_t lightIndex, glm::vec3 spawnLocation);
	
	void CreateGenericPlayer(Entity::SPtr playerEntity, size_t lightIndex, glm::vec3 spawnLocation, Netcode::PlayerID playerID);
	
	Entity::SPtr CreateBot(Model* boundingBoxModel, Model* characterModel, const glm::vec3& pos, Model* lightModel, size_t lightIndex, NodeSystem* ns);
	Entity::SPtr CreateStaticMapObject(const std::string& name, Model * model, Model* boundingBoxModel, const glm::vec3& pos = glm::vec3(0,0,0), const glm::vec3& rot = glm::vec3(0,0,0), const glm::vec3& scale = glm::vec3(1,1,1));
	Entity::SPtr CreateProjectile(Entity::SPtr projectileEntity,
		const glm::vec3& pos, const glm::vec3& velosity,
		bool hasLocalOwner = false, Netcode::ComponentID ownersNetId = 0,
		Netcode::ComponentID netCompId = 99999, float lifetime = 4);

	Entity::SPtr CreateScreenSpaceText(const std::string& text, glm::vec2 origin, glm::vec2 size);

	Entity::SPtr CreateGUIEntity(const std::string& name, const std::string& texture, glm::vec2 origin, glm::vec2 size);
}