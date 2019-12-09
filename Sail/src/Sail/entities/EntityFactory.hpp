#pragma once
#include "Sail/entities/ECS.h"

#include "Sail/netcode/NetcodeTypes.h"

class Model;
class NodeSystem;
class Shader;
class Camera;

namespace EntityFactory {
	struct ProjectileArguments {
		glm::vec3 pos;
		glm::vec3 velocity;
		bool hasLocalOwner = false;
		Netcode::ComponentID ownersNetId = 0;
		Netcode::ComponentID netCompId = 9999;
		float lifetime = 4.f;
	};




	void CreateCandle(Entity::SPtr& candle, const glm::vec3& lightPos, size_t lightIndex);
	
	Entity::SPtr CreateWaterGun(const std::string& name);
	
	void AddCandleComponentsToPlayer(Entity::SPtr& player, const size_t& lightIndex, const Netcode::PlayerID& playerID);

	Entity::SPtr CreateMyPlayer(Netcode::PlayerID playerID, size_t lightIndex, glm::vec3 spawnLocation);

	void CreateOtherPlayer(Entity::SPtr& otherPlayer, Netcode::PlayerID playerID, size_t lightIndex);
	
	Entity::SPtr CreateReplayPlayer(Netcode::ComponentID playerCompID, Netcode::ComponentID candleCompID,
		Netcode::ComponentID gunCompID, size_t lightIndex, glm::vec3 spawnLocation);

	void CreatePerformancePlayer(Entity::SPtr playerEnt, size_t lightIndex, glm::vec3 spawnLocation);
	
	void CreateGenericPlayer(Entity::SPtr playerEntity, size_t lightIndex, glm::vec3 spawnLocation, Netcode::PlayerID playerID, bool doNotAddToSystems = false);
	Entity::SPtr CreateMySpectator(Netcode::PlayerID playerID, size_t lightIndex, glm::vec3 spawnLocation);
	
	Entity::SPtr CreatePowerUp(glm::vec3& spawn, const int type, Netcode::ComponentID comID = 0);

	Entity::SPtr CreateBot(Model* boundingBoxModel, Model* characterModel, const glm::vec3& pos, Model* lightModel, size_t lightIndex, NodeSystem* ns);
	Entity::SPtr CreateCleaningBot(const glm::vec3& pos, NodeSystem* ns);
	Entity::SPtr CreateStaticMapObject(const std::string& name, Model * model, Model* boundingBoxModel, const glm::vec3& pos = glm::vec3(0,0,0), const glm::vec3& rot = glm::vec3(0,0,0), const glm::vec3& scale = glm::vec3(1,1,1));
	
	Entity::SPtr CreateProjectile(Entity::SPtr projectileEntity, const ProjectileArguments& info);
	Entity::SPtr CreateReplayProjectile(Entity::SPtr projectileEntity, const ProjectileArguments& info);

	Entity::SPtr CreateScreenSpaceText(const std::string& text, glm::vec2 origin, glm::vec2 size);

	Entity::SPtr CreateGUIEntity(const std::string& name, const std::string& texture, glm::vec2 origin, glm::vec2 size);

	Entity::SPtr CreateCrosshairEntity(const std::string& name);
}